/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <iostream>

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "common/string.h"
#include "mud/fileobj.h"
#include "common/log.h"
#include "mud/uniqid.h"
#include "scriptix/native.h"

using namespace std;

#define VALID_NAME_CHAR(c) (isalnum(c) || (c) == '_')

namespace {
	struct EscapeString
	{
		inline EscapeString (const String& s_string) : string(s_string) {}

		inline friend ostream& operator << (ostream& os, const EscapeString& esc)
		{
			os << '"';
			for (String::const_iterator i = esc.string.begin(); i != esc.string.end(); ++i) {
				switch (*i) {
					case '\t':
						os << "\\t";
						break;
					case '\n':
						os << "\\n";
						break;
					case '\\':
						os << "\\\\";
						break;
					case '"':
						os << "\\\"";
						break;
					case '\0':
						break;
					default:
						os << *i;
				}
			}
			os << '"';

			return os;
		}

		const String& string;
	};

	struct ScrubString
	{
		inline ScrubString (const String& s_string) : string(s_string) {}

		inline friend ostream& operator << (ostream& os, const ScrubString& esc)
		{
			os << '"';
			for (String::const_iterator i = esc.string.begin(); i != esc.string.end(); ++i) {
				if (isalpha(*i) || (isdigit(*i) && i != esc.string.begin()) || *i == '_')
					os << *i;
			}

			return os;
		}

		const String& string;
	};
}

int
File::Reader::open (String filename)
{
	// open
	in.open(filename.c_str());
	if (!in) {
		Log::Error << "Failed to open " << filename;
		return -1;
	}
	// finish
	Reader::filename = filename;
	line = 1;
	return 0;
}

File::Reader::Token
File::Reader::read_token (String& outstr)
{
	int test;

	// loop until we actually have a token
	do {
		// consume whitespace
		while (in && isspace(in.peek()))
			if (in.get() == '\n')
				++line;

		// determine type
		test = in.get();

		// end of file
		if (test == -1)
			return TOKEN_EOF;

		// comment?  eat to end of line
		if (test == '#') {
			while (in.get() != '\n')
				;
			++line;
		}
	} while (test == '#');

	// various symbols
	if (test == '{') {
		return TOKEN_BEGIN;
	} else if (test == '}') {
		return TOKEN_END;
	} else if (test == '=') {
		return TOKEN_SET;
	} else if (test == '.') {
		return TOKEN_KEY;
	} else if (test == '[') {
		return TOKEN_START_LIST;
	} else if (test == ']') {
		return TOKEN_END_LIST;
	} else if (test == ',') {
		return TOKEN_COMMA;

	// string
	} else if (test == '"') {
		StringBuffer data;
		// read in string
		while (in && (test = in.peek()) != '"') {
			// line breaks not allowed
			if (test == '\n') {
				outstr = S("Line break in string");
				return TOKEN_ERROR;

			// escape?
			} else if (test == '\\') {
				in.get();
				test = in.peek();
				if (test == 'n')
					data << '\n';
				else if (test == 't')
					data << '\t';
				else if (test == '\\')
					data << '\\';
				else if (test == '"')
					data << '"';
				else {
					outstr = S("Unknown escape code");
					return TOKEN_ERROR;
				}

				// consume the op
				in.get();

			// normal creature
			} else {
				data << (char)in.get();
			}
		}

		// consume the "
		in.get();

		outstr = data.str();
		return TOKEN_STRING;

	// number
	} else if (isdigit(test) || test == '-') {
		StringBuffer data;
		// read in name
		data << (char)test;
		while (in && isdigit(in.peek()))
			data << (char)in.get();

		outstr = data.str();
		return TOKEN_NUMBER;

	// ID
	} else if (test == '<') {
		StringBuffer data;
		while (in && isdigit(in.peek()))
			data << (char)in.get();
		if (in.get() != '>') {
			outstr = S("Invalid ID");
			return TOKEN_ERROR;
		}

		outstr = data.str();

		return TOKEN_ID;

	// name
	} else if (isalpha(test) || test == '_') {
		StringBuffer data;
		// read in name
		data << (char)test;
		while (in) {
			test = in.peek();
			if (!isalnum(test) && test != '_')
				break;
			data << (char)in.get();
		}

		outstr = data.str();

		// true or false?  we're a bool
		if (outstr == "true")
			return TOKEN_TRUE;
		if (outstr == "false")
			return TOKEN_FALSE;

		// begin?  we're a block
		if (outstr == "begin") {
			// must have whitespace to end of line
			while (in && in.peek() != '\n')
				if (!isspace(in.get())) {
					outstr = S("Syntax error: garbage after <<[");
					return TOKEN_ERROR;
				}
			in.get();
			++line;

			// get block
			outstr.clear();
			while (in) {
				// read a line
				StringBuffer data;
				do {
					char ch = in.get();
					data << ch;
					if (ch == '\n') {
						++ line;
						break;
					}
				} while (in);

				// line end pattern?
				String tstr = data.str();
				if (strstr(tstr, "end") && strip(tstr) == "end") // see if the string exists, if so, see if that's all there is
					break;

				// add data
				outstr = outstr + tstr;
			}

			return TOKEN_STRING;
		}

		// we're a gneric name token
		return TOKEN_NAME;
	}

	// error
	StringBuffer data;
	data << (char)test;
	while (in && !isspace(in.peek()))
		data << (char)in.get();

	outstr = S("Syntax error: unknown symbol: ") + data.str();
	return TOKEN_ERROR;
}

bool
File::Reader::get (Node& node)
{
	Token op;
	String opstr;
	GCType::vector<Value> list;

	// clear node
	node.klass.clear();
	node.name.clear();
	node.value = Value();
	node.type = Node::ERROR;

	// get op - expect name
	op = read_token(opstr);

	// end of file - ok
	if (op == TOKEN_EOF)
		return false;

	// error
	if (op == TOKEN_ERROR)
		throw File::Error(opstr);

	// end of object?
	if (op == TOKEN_END) {
		// set node
		node.type = Node::END;
		return true;
	}

	// set line
	node.line = line;

	// expect a name
	if (op != TOKEN_NAME)
		throw File::Error(S("Parse error: name expected"));
	node.klass = opstr;

	// get next token
	op = read_token(opstr);
	if (op == TOKEN_ERROR)
		throw File::Error(opstr);
	if (op == TOKEN_EOF)
		throw File::Error(S("Parse error: unexpected EOF"));

	// keyed attr?
	if (op == TOKEN_KEY) {
		// read key
		op = read_token(opstr);
		if (op != TOKEN_NAME && op != TOKEN_STRING)
			throw File::Error(S("Parse error: name expected after ."));
		node.name = opstr;

		// next token
		op = read_token(opstr);
		if (op == TOKEN_ERROR)
			throw File::Error(opstr);
		if (op == TOKEN_EOF)
			throw File::Error(S("Parse error: unexpected EOF"));
	}

	// object?
	if (op == TOKEN_BEGIN) {
		// if we had a name, we can't be an object
		if (node.name)
			throw File::Error(S("Parse error: unexpected object"));

		// return as object type
		node.type = Node::BEGIN;
		return true;
	}

	// attribute?
	else if (op == TOKEN_SET) {
		// attributes must have a key
		if (!node.name)
			throw File::Error(S("Parse error: name expected before ="));

		// read
		String data;
		Token type = read_token(data);

		// handle errors
		if (type == TOKEN_ERROR)
			throw File::Error(data);
		if (type == TOKEN_EOF)
			throw File::Error(S("Parse error: unexpected EOF"));

		// attribute-object
		if (type == TOKEN_NAME) {
			node.value = Value(Value::TYPE_STRING, data);
			type = read_token(data);
			if (type != TOKEN_BEGIN)
				throw File::Error(S("Parse error: { expected after name"));
			node.type = Node::ABEGIN;
			return true;
		}

		// can we read the value?
		if (!set_value(type, data, node.value)) {
			// unknown type
			throw File::Error(S("Parse error: invalid attribute value"));
		}

		node.type = Node::ATTR;
		return true;
	}

	// wrong token
	throw File::Error(S("Parse error: unexpected token"));
}

bool
File::Reader::set_value (File::Reader::Token type, String data, File::Value& value)
{
	if (type == TOKEN_NUMBER) {
		value = Value(Value::TYPE_INT, data);
	} else if (type == TOKEN_TRUE) {
		value = Value(Value::TYPE_BOOL, S("true"));
	} else if (type == TOKEN_FALSE) {
		value = Value(Value::TYPE_BOOL, S("false"));
	} else if (type == TOKEN_STRING) {
		value = Value(Value::TYPE_STRING, data);
	} else if (type == TOKEN_ID) {
		value = Value(Value::TYPE_ID, data);
	} else if (type == TOKEN_START_LIST) {
		GCType::vector<Value> list;
		while (true) {
			type = read_token(data);
			if (type == TOKEN_ERROR)
				throw File::Error(data);
			Value nvalue;
			if (type == TOKEN_START_LIST) {
				Log::Error << "Illegal to include a list in a list at " << get_filename() << ":" << get_line();
				return false;
			}
			if (!set_value(type, data, nvalue)) {
				Log::Error << "Unexpected token in list at " << get_filename() << ":" << get_line();
				return false;
			}
			list.push_back(nvalue);
			type = read_token(data);
			if (type == TOKEN_ERROR)
				throw File::Error(data);
			else if (type == TOKEN_END_LIST)
				break;
			else if (type != TOKEN_COMMA) {
				Log::Error << "Unexpected token in list at " << get_filename() << ":" << get_line();
				return false;
			}
		}
		value = Value(list);
	} else {
		return false;
	}
	return true;
}

void
File::Reader::consume ()
{
	Node node(*this);
	size_t depth = 0;

	// keep reading nodes until EOF or begin end
	while (get(node)) {
		// increment depth on new node
		if (node.is_begin())
			++depth;
		// decrement depth on end, and abort if we have depth 0 on end
		else if (node.is_end())
			if (depth-- == 0)
				break;
	}
}

int
File::Writer::open (String filename)
{
	// open
	out.open(filename.c_str());
	if (!out) {
		Log::Error << "Failed to open " << filename;
		return -1;
	}
	indent = 0;
	return 0;
}

void
File::Writer::close ()
{
	if (out) {
		out << "# vim: set shiftwidth=2 tabstop=2 expandtab:\n";
		out.close();
	}
}

void
File::Writer::do_indent ()
{
	for (unsigned long i = 0; i < indent; ++i)
		out << "  ";
}

void
File::Writer::attr (String klass, String name, String data)
{
	if (!out)
		return;

	if (is_begin_open) {
		Log::Error << "Open begin block not followed by object";
		return;
	}

	if (!File::valid_name(klass) || !File::valid_name(name)) {
		Log::Error << "Attempted to write id '" << klass << "." << name << "'";
		return;
	}

	do_indent();
	out << klass << "." << name << " = " << EscapeString(data) << "\n";
}

void
File::Writer::attr (String klass, String name, long data)
{
	if (!out)
		return;

	if (!File::valid_name(klass) || !File::valid_name(name)) {
		Log::Error << "Attempted to write id '" << klass << "." << name << "'";
		return;
	}

	do_indent();
	out << klass << "." << name << " = " << data << "\n";
}

void
File::Writer::attr (String klass, String name, bool data)
{
	if (!out)
		return;

	if (!File::valid_name(klass) || !File::valid_name(name)) {
		Log::Error << "Attempted to write id '" << klass << "." << name << "'";
		return;
	}

	do_indent();
	out << klass << "." << name << " = " << (data ? "true" : "false") << "\n";
}

void
File::Writer::attr (String klass, String name, const UniqueID& data)
{
	if (!out)
		return;

	if (is_begin_open) {
		Log::Error << "Open begin block not followed by object";
		return;
	}

	if (!File::valid_name(klass) || !File::valid_name(name)) {
		Log::Error << "Attempted to write id '" << klass << "." << name << "'";
		return;
	}

	do_indent();

	out << klass << "." << name << " = <" << UniqueIDManager.encode(data) << ">\n";
}

void
File::Writer::attr (String klass, String name, const GCType::vector<Value>& list)
{
	if (!out)
		return;

	if (is_begin_open) {
		Log::Error << "Open begin block not followed by object";
		return;
	}

	if (!File::valid_name(klass) || !File::valid_name(name)) {
		Log::Error << "Attempted to write id '" << klass << "." << name << "'";
		return;
	}

	out << klass << "." << name << " = [ ";

	for (GCType::vector<Value>::const_iterator i = list.begin(); i != list.end(); ++i) {
		if (i != list.begin())
			out << ", ";
		switch (i->get_type()) {
			case Value::TYPE_NONE:
			case Value::TYPE_LIST:
				out << "\"\"";
				break;
			case Value::TYPE_STRING:
				out << EscapeString(i->get_value());
				break;
			case Value::TYPE_INT:
			case Value::TYPE_BOOL:
			case Value::TYPE_ID:
				out << i->get_value();
				break;
		}
	}
	out << " ]\n";
}

void
File::Writer::block (String klass, String name, String data)
{
	if (!out)
		return;

	if (is_begin_open) {
		Log::Error << "Open begin block not followed by object";
		return;
	}

	if (!File::valid_name(klass) || !File::valid_name(name)) {
		Log::Error << "Attempted to write id '" << klass << "." << name << "'";
		return;
	}

	do_indent();

	// beginning
	out << klass << "." << name << " = begin\n" << data;
	// we need to add a newline if we don't have one on end already
	// FIXME: escape if data includes end line
	if (data.empty() || data[data.size()-1] != '\n')
		out << '\n';
	// ending
	do_indent();
	out << "end\n";
}

void
File::Writer::begin (String klass)
{
	if (!out)
		return;

	if (!File::valid_name(klass)) {
		Log::Error << "Attempted to write id '" << klass << "'";
		return;
	}

	if (!is_begin_open)
		do_indent();
	out << klass << " {\n";
	++ indent;
	is_begin_open = false;
}

void
File::Writer::begin_open (String klass, String name)
{
	if (!out)
		return;

	if (is_begin_open) {
		Log::Error << "Open begin block not followed by object";
		return;
	}

	if (!File::valid_name(klass) || !File::valid_name(name)) {
		Log::Error << "Attempted to write id '" << klass << "." << name << "' {";
		return;
	}

	do_indent();
	out << klass << '.' << name << " = ";
	is_begin_open = true;
}

void
File::Writer::end ()
{
	if (!out)
		return;

	// unindent, then write
	-- indent;
	do_indent();
	out << "}\n";
}

void
File::Writer::comment (String text)
{
	if (!out)
		return;
	if (text.empty())
		return;

	do_indent();

	// make sure we format comments with newlines properly
	out << "# ";
	const char* tptr = text.c_str();
	while (*tptr != '\0') {
		if (*tptr == '\n') {
			out << '\n';
			do_indent();
			out << "# ";
		} else {
			out << *tptr;
		}
		++ tptr;
	}
	out << '\n';
}

bool
File::valid_name (String name)
{
	// not empty
	if (name.empty())
		return false;

	// must start with a letter or _
	if (!isalpha(name[0]) && name[0] != '_')
		return false;

	// must be alpha, digit, _
	for (String::const_iterator i = name.begin(); i != name.end(); ++i)
		if (!isalnum(*i) && *i != '_')
			return false;

	// all good
	return true;
}

int
File::Node::get_int () const
{
	if (value.get_type() != Value::TYPE_INT) {
		Log::Error << "Incorrect data type for '" << get_class() << '.' << get_name() << "' at :" << get_line();
		throw(File::Error(S("data type mismatch")));
	}

	return tolong(value.get_value());
}

bool
File::Node::get_bool () const
{
	if (value.get_type() != Value::TYPE_BOOL) {
		Log::Error << "Incorrect data type for '" << get_class() << '.' << get_name() << "' at :" << get_line();
		throw(File::Error(S("data type mismatch")));
	}
	return value.get_value() == "true" || value.get_value() == "yes" || value.get_value() == "on";
}

String
File::Node::get_string () const
{
	if (value.get_type() != Value::TYPE_STRING) {
		Log::Error << "Incorrect data type for '" << get_class () << '.' << get_name() << "' at :" << get_line();
		throw(File::Error(S("data type mismatch")));
	}
	return value.get_value();
}

UniqueID
File::Node::get_id () const
{
	if (value.get_type() != Value::TYPE_ID) {
		Log::Error << "Incorrect data type for '" << get_class() << '.' << get_name() << "' at :" << get_line();
		throw(File::Error(S("data type mismatch")));
	}
	return UniqueIDManager.decode(value.get_value());
}

const GCType::vector<File::Value>&
File::Node::get_list () const
{
	if (value.get_type() != Value::TYPE_LIST) {
		Log::Error << "Incorrect data type for '" << get_class() << '.' << get_name() << "' at :" << get_line();
		throw(File::Error(S("data type mismatch")));
	}
	return value.get_list();
}

const GCType::vector<File::Value>&
File::Node::get_list (size_t size) const
{
	if (value.get_type() != Value::TYPE_LIST) {
		Log::Error << "Incorrect data type for '" << get_class() << '.' << get_name() << "' at :" << get_line();
		throw(File::Error(S("data type mismatch")));
	}
	if (value.get_list().size() != size) {
		Log::Error << "Incorrect number of elements in list for '" << get_class() << '.' << get_name() << "' at :" << get_line();
		throw(File::Error(S("list size mismatch")));
	}
	return value.get_list();
}

String
File::Node::get_string (size_t index) const
{
	if (value.get_type() != Value::TYPE_LIST) {
		Log::Error << "Incorrect data type for '" << get_class () << '.' << get_name() << "' at :" << get_line();
		throw(File::Error(S("data type mismatch")));
	}
	if (value.get_list().size() <= index) {
		Log::Error << "Incorrect number of elements in list for '" << get_class() << '.' << get_name() << "' at :" << get_line();
		throw(File::Error(S("list size mismatch")));
	}
	if (value.get_list()[index].get_type() != Value::TYPE_STRING) {
		Log::Error << "Incorrect data type for element " << (index + 1) << " of '" << get_class () << '.' << get_name() << "' at :" << get_line();
		throw(File::Error(S("data type mismatch")));
	}
	return value.get_list()[index].get_value();
}

int
File::Node::get_int (size_t index) const
{
	if (value.get_type() != Value::TYPE_LIST) {
		Log::Error << "Incorrect data type for '" << get_class () << '.' << get_name() << "' at :" << get_line();
		throw(File::Error(S("data type mismatch")));
	}
	if (value.get_list().size() <= index) {
		Log::Error << "Incorrect number of elements in list for '" << get_class() << '.' << get_name() << "' at :" << get_line();
		throw(File::Error(S("list size mismatch")));
	}
	if (value.get_list()[index].get_type() != Value::TYPE_INT) {
		Log::Error << "Incorrect data type for element " << (index + 1) << " of '" << get_class () << '.' << get_name() << "' at :" << get_line();
		throw(File::Error(S("data type mismatch")));
	}
	return tolong(value.get_list()[index].get_value());
}

const StreamControl&
operator<< (const StreamControl& stream, const File::Node& node)
{
	stream << node.get_reader().get_path() << ',' << node.get_line() << ": ";
	switch (node.get_node_type()) {
		case File::Node::ATTR:
			stream << node.get_class() << '.' << node.get_name() << " (";
			switch (node.get_value_type()) {
				case File::Value::TYPE_NONE: stream << "nil"; break;
				case File::Value::TYPE_STRING: stream << "string"; break;
				case File::Value::TYPE_INT: stream << "int"; break;
				case File::Value::TYPE_BOOL: stream << (node.get_bool()?"true":"false"); break;
				case File::Value::TYPE_ID: stream << "id"; break;
				case File::Value::TYPE_LIST: stream << "list"; break;
				default: stream << "unknown"; break;
			}
			stream << ')';
			break;
		case File::Node::ABEGIN:
			stream << node.get_class() << '.' << node.get_name() << " (" << node.get_string() << ')';
			break;
		case File::Node::BEGIN:
			stream << node.get_class();
			break;
		case File::Node::END:
			stream << "$eof";
			break;
		case File::Node::ERROR:
			stream << "<error>";
			break;
		default:
			stream << "unknown";
			break;
	}

	return stream;
}

SCRIPT_TYPE(RestrictedWriter)
ScriptRestrictedWriter::ScriptRestrictedWriter (File::Writer* s_writer) :
    Scriptix::Native(AweMUD_RestrictedWriterType),
    writer(s_writer)
{}
