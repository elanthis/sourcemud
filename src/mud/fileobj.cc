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

			// normal character
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
		}

		// outstr = strlower(outstr);

		return TOKEN_STRING;
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

	// clear node
	node.name.clear();
	node.key.clear();
	node.data.clear();

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
	if (op != TOKEN_STRING)
		throw File::Error(S("Parse error: name expected"));
	node.name = opstr;

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
		if (op != TOKEN_STRING)
			throw File::Error(S("Parse error: name expected after :"));
		node.key = opstr;

		// next token
		op = read_token(opstr);
		if (op == TOKEN_ERROR)
			throw File::Error(opstr);
		if (op == TOKEN_EOF)
			throw File::Error(S("Parse error: unexpected EOF"));
	}

	// object?
	if (op == TOKEN_BEGIN) {
		// if we had a key, we can't be an object
		if (node.key)
			throw File::Error(S("Parse error: unexpected object"));

		// return as object type
		node.type = Node::BEGIN;
		return true;
	}

	// attribute?
	else if (op == TOKEN_SET) {
		// read
		node.type = node.key ? Node::KEYED : Node::ATTR;
		String data;
		Token type = read_token(data);

		// handle errors
		if (type == TOKEN_ERROR)
			throw File::Error(data);
		if (type == TOKEN_EOF)
			throw File::Error(S("Parse error: unexpected EOF"));

		// set data
		if (type == TOKEN_NUMBER) {
			node.datatype = TYPE_INT;
			node.data = data;
		} else if (type == TOKEN_TRUE) {
			node.datatype = TYPE_BOOL;
			node.data = S("true");
		} else if (type == TOKEN_FALSE) {
			node.datatype = TYPE_BOOL;
			node.data = S("false");
		} else if (type == TOKEN_STRING) {
			node.datatype = TYPE_STRING;
			node.data = data;
		} else if (type == TOKEN_ID) {
			node.datatype = TYPE_ID;
			node.data = data;
		// invalid syntax
		} else {
			throw File::Error(S("Parse error: invalid syntax"));
		}

		return true;
	}

	// wrong token
	throw File::Error(S("Parse error: unexpected token"));
}

void
File::Reader::consume (void)
{
	Node node;
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
File::Object::load (File::Reader& reader)
{
	File::Node node;

	try {
		while (reader.get(node)) {
			if (node.is_end()) {
				break;
			} else if (!node.is_attr()) {
				throw File::Error(S("Invalid declaration in object"));
			}

			if (node.get_datatype() == TYPE_STRING)
				strings[node.get_name()].push_back(node.get_string());
			else if (node.get_datatype() == TYPE_INT)
				ints[node.get_name()].push_back(node.get_int());
			else if (node.get_datatype() == TYPE_BOOL)
				bools[node.get_name()].push_back(node.get_bool());
			else if (node.get_datatype() == TYPE_ID)
				ids[node.get_name()].push_back(node.get_id());
			else
				throw File::Error(S("Unknown data type in object"));
		}
		return 0;
	} catch (File::Error& error) {
		Log::Error << error.get_what() << " at " << reader.get_filename() << ':' << reader.get_line();
		return -1;
	}
}

String
File::Object::get_string (String name, uint index) const
{
	StringList::const_iterator i = strings.find(name);
	if (i == strings.end())
		throw KeyError(TYPE_STRING, name, index);
	if (index > i->second.size())
		throw KeyError(TYPE_STRING, name, index);
	return i->second[index];
}

File::KeyError::KeyError (DataType type, String name, uint index) : File::Error(String())
{
	what = (type == TYPE_STRING ? S("string") :
		type == TYPE_INT ? S("int") :
		type == TYPE_BOOL ? S("bool") :
		type == TYPE_ID ? S("id") :
		S("unknown"))
		+ " " + name + "." + tostr(index) + " is not defined";
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
File::Writer::close (void)
{
	if (out) {
		out << "# vim: set shiftwidth=2 tabstop=2 expandtab:\n";
		out.close();
	}
}

void
File::Writer::do_indent (void)
{
	for (unsigned long i = 0; i < indent; ++i)
		out << "  ";
}

void
File::Writer::attr (String name, String data)
{
	if (!out)
		return;

	if (!File::valid_name(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();

	// output
	out << name << " = " << EscapeString(data) << "\n";
}

void
File::Writer::attr (String name, long data)
{
	if (!out)
		return;

	if (!File::valid_name(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();
	out << name << " = " << data << "\n";
}

void
File::Writer::attr (String name, long long data)
{
	if (!out)
		return;

	if (!File::valid_name(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();
	out << name << " = " << data << "\n";
}

void
File::Writer::attr (String name, bool data)
{
	if (!out)
		return;

	if (!File::valid_name(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();

	out << name << " = " << (data ? "true" : "false") << "\n";
}

void
File::Writer::attr (String name, const UniqueID& data)
{
	if (!out)
		return;

	if (!File::valid_name(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();

	out << name << " = <" << UniqueIDManager.encode(data) << ">\n";
}

void
File::Writer::keyed (String name, String key, String data)
{
	if (!out)
		return;

	if (!File::valid_name(name) || !File::valid_name(key)) {
		Log::Error << "Attempted to write id '" << name << "." << key << "'";
		return;
	}

	do_indent();
	out << name << "." << key << " = " << EscapeString(data) << "\n";
}

void
File::Writer::keyed (String name, String key, long data)
{
	if (!out)
		return;

	if (!File::valid_name(name) || !File::valid_name(key)) {
		Log::Error << "Attempted to write id '" << name << "." << key << "'";
		return;
	}

	do_indent();
	out << name << "." << key << " = " << data << "\n";
}

void
File::Writer::keyed (String name, String key, bool data)
{
	if (!out)
		return;

	if (!File::valid_name(name) || !File::valid_name(key)) {
		Log::Error << "Attempted to write id '" << name << "." << key << "'";
		return;
	}

	do_indent();
	out << name << "." << key << " = " << (data ? "true" : "false") << "\n";
}

void
File::Writer::keyed (String name, String key, const UniqueID& data)
{
	if (!out)
		return;

	if (!File::valid_name(name) || !File::valid_name(key)) {
		Log::Error << "Attempted to write id '" << name << "." << key << "'";
		return;
	}

	do_indent();

	out << name << "." << key << " = <" << UniqueIDManager.encode(data) << ">\n";
}

void
File::Writer::block (String name, String data)
{
	if (!out)
		return;

	if (!File::valid_name(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();

	// beginning
	out << name << " = begin\n" << data;
	// we need to add a newline if we don't have one on end already
	// FIXME: escape if data includes end line
	if (data.empty() || data[data.size()-1] != '\n')
		out << '\n';
	// ending
	do_indent();
	out << "end\n";
}

void
File::Writer::begin (String name)
{
	if (!out)
		return;

	if (!File::valid_name(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();
	out << name << " {\n";
	++ indent;
}

void
File::Writer::end (void)
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
	if (datatype != TYPE_INT) {
		Log::Error << "Incorrect data type for '" << get_name() << "' at :" << get_line();
		throw(File::Error(S("data type mismatch")));
	}

	return tolong(data);
}

bool
File::Node::get_bool () const
{
	if (datatype != TYPE_BOOL) {
		Log::Error << "Incorrect data type for '" << get_name() << "' at :" << get_line();
		throw(File::Error(S("data type mismatch")));
	}
	return data == "true" || data == "yes" || data == "on";
}

String
File::Node::get_string () const
{
	if (datatype != TYPE_STRING) {
		Log::Error << "Incorrect data type for '" << get_name() << "' at :" << get_line();
		throw(File::Error(S("data type mismatch")));
	}
	return data;
}

UniqueID
File::Node::get_id () const
{
	if (datatype != TYPE_ID) {
		Log::Error << "Incorrect data type for '" << get_name() << "' at :" << get_line();
		throw(File::Error(S("data type mismatch")));
	}
	return UniqueIDManager.decode(data);
}

SCRIPT_TYPE(RestrictedWriter)
ScriptRestrictedWriter::ScriptRestrictedWriter (File::Writer* s_writer) :
    Scriptix::Native(AweMUD_RestrictedWriterType),
    writer(s_writer)
{}
