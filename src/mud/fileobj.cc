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

#define VALID_NAME_CHAR(c) (isalpha(c) || isdigit(c) || (c) == '_' || (c) == '-' || (c) == '.')

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
}

int
File::Reader::open (StringArg filename)
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
	StringBuffer data;
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
	} else if (test == '@') {
		return TOKEN_CUSTOM;
	} else if (test == ':') {
		return TOKEN_KEY;

	// block symbol
	} else if (test == '<') {
		// must be <<[
		if (in.peek() != '<') {
			outstr = "Syntax error: <?";
			return TOKEN_ERROR;
		}
		in.get();
		if (in.peek() != '[') {
			outstr = "Syntax error: <<?";
			return TOKEN_ERROR;
		}
		in.get();

		// must have whitespace to end of line
		while (in && in.peek() != '\n')
			if (!isspace(in.get())) {
				outstr = "Syntax error: garbage after <<[";
				return TOKEN_ERROR;
			}
		in.get();
		++line;

		// get block
		while (in) {
			// read a line
			data.str("");
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
			size_t start = tstr.find_first_not_of(" \t\n");
			if (start != String::npos) {
				size_t back = tstr.find_last_not_of(" \t\n");
				if (!strncmp("]>>", tstr.c_str() + start, back - start + 1))
					break;
			}

			// add data
			outstr = outstr + data.str();
		}

		return TOKEN_STRING;
	// string
	} else if (test == '"') {
		// read in string
		while (in && (test = in.peek()) != '"') {
			// line breaks not allowed
			if (test == '\n') {
				outstr = "Line break in string";
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
					outstr = "Unknown escape code";
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
		// read in name
		data << (char)test;
		while (in && isdigit(in.peek()))
			data << (char)in.get();

		outstr = data.str();
		return TOKEN_NUMBER;

	// name
	} else if (isalpha(test)) {
		// read in name
		data << (char)test;
		while (in) {
			test = in.peek();
			if (!VALID_NAME_CHAR(test))
				break;
			data << (char)in.get();
		}

		outstr = data.str();

		// true or false?  we're a bool
		if (outstr == "true")
			return TOKEN_TRUE;
		if (outstr == "false")
			return TOKEN_FALSE;

		// outstr = strlower(outstr);

		return TOKEN_STRING;

	// unique ID
	} else if (test == '$') {
		while (in && !isspace(in.peek()))
			data << (char)in.get();

		outstr = data.str();

		return TOKEN_ID;
	}

	// errror
	data << (char)test;
	while (in && !isspace(in.peek()))
		data << (char)in.get();

	outstr = "Syntax error: unknown symbol: " + data.str();
	return TOKEN_ERROR;
}

bool
File::Reader::get (Node& node)
{
	Token op;
	String opstr;
	bool custom;

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

	// get a custom (@) operator?
	custom = false;
	if (op == TOKEN_CUSTOM) {
		custom = true;
		op = read_token(opstr);
	}

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
		// custom attrs don't have keys
		if (custom)
			throw File::Error(S("Parse error: unexpected :"));

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
		// if we had a custom marker or key, we can't be an object
		if (custom || node.key)
			throw File::Error(S("Parse error: unexpected object"));

		// return as object type
		node.type = Node::BEGIN;
		return true;
	}

	// attribute?
	else if (op == TOKEN_SET) {
		// read
		node.type = custom ? Node::CUSTOM : (node.key ? Node::KEYED : Node::ATTR);
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
			node.data = "true";
		} else if (type == TOKEN_FALSE) {
			node.datatype = TYPE_BOOL;
			node.data = "false";
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
File::Writer::open (StringArg filename)
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
File::Writer::attr (StringArg name, StringArg data)
{
	if (!out)
		return;

	if (!str_is_valid_id(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();

	// output
	out << name << " = " << EscapeString(data) << "\n";
}

void
File::Writer::attr (StringArg name, long data)
{
	if (!out)
		return;

	if (!str_is_valid_id(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();
	out << name << " = " << data << "\n";
}

void
File::Writer::attr (StringArg name, long long data)
{
	if (!out)
		return;

	if (!str_is_valid_id(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();
	out << name << " = " << data << "\n";
}

void
File::Writer::attr (StringArg name, bool data)
{
	if (!out)
		return;

	if (!str_is_valid_id(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();

	out << name << " = " << (data ? "true" : "false") << "\n";
}

void
File::Writer::attr (StringArg name, const UniqueID& data)
{
	if (!out)
		return;

	if (!str_is_valid_id(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();

	out << name << " = $" << UniqueIDManager.encode(data) << "\n";
}

void
File::Writer::keyed (StringArg name, StringArg key, StringArg data)
{
	if (!out)
		return;

	if (!str_is_valid_id(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();
	out << name << " : " << EscapeString(key) << " = " << EscapeString(data) << "\n";
}

void
File::Writer::keyed (StringArg name, StringArg key, long data)
{
	if (!out)
		return;

	if (!str_is_valid_id(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();
	out << name << " : " << EscapeString(key) << " = " << data << "\n";
}

void
File::Writer::keyed (StringArg name, StringArg key, bool data)
{
	if (!out)
		return;

	if (!str_is_valid_id(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();
	out << name << " : " << EscapeString(key) << " = " << (data ? "true" : "false") << "\n";
}

void
File::Writer::keyed (StringArg name, StringArg key, const UniqueID& data)
{
	if (!out)
		return;

	if (!str_is_valid_id(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();

	out << name << " : " << EscapeString(key) << " = $" << UniqueIDManager.encode(data) << "\n";
}

void
File::Writer::custom (StringArg name, StringArg data)
{
	if (!out)
		return;

	if (!str_is_valid_id(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();

	// name
	out << '@' << name << " = \"";

	for (String::const_iterator i = data.begin(); i != data.end(); ++i) {
		switch (*i) {
			case '\t':
				out << "\\t";
				break;
			case '\n':
				out << "\\n";
				break;
			case '\\':
				out << "\\\\";
				break;
			case '"':
				out << "\\\"";
				break;
			case '\0':
				break;
			default:
				out << *i;
		}
	}

	// newline
	out << "\"\n";
}

void
File::Writer::custom (StringArg name, long data)
{
	if (!out)
		return;

	if (!str_is_valid_id(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();
	out << '@' << name << " = " << data << "\n";
}

void
File::Writer::custom (StringArg name, bool data)
{
	if (!out)
		return;

	if (!str_is_valid_id(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();

	out << '@' << name << " = " << (data ? "true" : "false") << "\n";
}

void
File::Writer::custom (StringArg name, const UniqueID& data)
{
	if (!out)
		return;

	if (!str_is_valid_id(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();

	out << '@' << name << " = &" << UniqueIDManager.encode(data) << "\n";
}

void
File::Writer::block (StringArg name, StringArg data)
{
	if (!out)
		return;

	if (!str_is_valid_id(name)) {
		Log::Error << "Attempted to write id '" << name << "'";
		return;
	}

	do_indent();

	// beginning
	out << name << " = <<[\n" << data;
	// we need to add a newline if we don't have one on end already
	// FIXME: escape ]>> if it's in the data
	if (data.empty() || data[data.size()-1] != '\n')
		out << '\n';
	// ending
	do_indent();
	out << "]>>\n";
}

void
File::Writer::begin (StringArg name)
{
	if (!out)
		return;

	if (!str_is_valid_id(name)) {
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
File::Writer::comment (StringArg text)
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
File::valid_name (StringArg name)
{
	// not empty
	if (name.empty())
		return false;

	// must start with a letter
	if (!isalpha(name[0]))
		return false;

	// must be alpha, digit, _, or .
	for (String::const_iterator i = name.begin(); i != name.end(); ++i)
		if (!VALID_NAME_CHAR(*i))
			return false;

	// all good
	return true;
}

int
File::Node::get_int () const
{
	if (datatype != TYPE_INT) {
		// FIXME: Log::Error << "Incorrect data type for '" << get_name() << "' at " << get_filename() << ':' << get_line();
		Log::Error << "Incorrect data type for '" << get_name() << "' at :" << get_line();
		throw(File::Error(S("data type mismatch")));
	}

	return tolong(data);
}

bool
File::Node::get_bool () const
{
	return data == "true" || data == "yes" || data == "on";
}

SCRIPT_TYPE(RestrictedWriter)
ScriptRestrictedWriter::ScriptRestrictedWriter (File::Writer* s_writer) :
    Scriptix::Native(AweMUD_RestrictedWriterType),
    writer(s_writer)
{}
