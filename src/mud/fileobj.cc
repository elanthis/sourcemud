/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/string.h"
#include "common/log.h"
#include "common/file.h"
#include "mud/fileobj.h"

using namespace std;

#define VALID_NAME_CHAR(c) (isalnum(c) || (c) == '_')

namespace
{
	struct EscapeString {
		inline EscapeString(const std::string& s_string) : string(s_string) {}

		inline friend ostream& operator << (ostream& os, const EscapeString& esc) {
			os << '"';
			for (std::string::const_iterator i = esc.string.begin(); i != esc.string.end(); ++i) {
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

		const std::string& string;
	};

	struct ScrubString {
		inline ScrubString(const std::string& s_string) : string(s_string) {}

		inline friend ostream& operator << (ostream& os, const ScrubString& esc) {
			os << '"';
			for (std::string::const_iterator i = esc.string.begin(); i != esc.string.end(); ++i) {
				if (isalpha(*i) || (isdigit(*i) && i != esc.string.begin()) || *i == '_')
					os << *i;
			}

			return os;
		}

		const std::string& string;
	};
}

int
File::Reader::open(const std::string& filename)
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

File::Reader::Token File::Reader::readToken(std::string& outstr)
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
		std::ostringstream data;
		// read in string
		while (in && (test = in.peek()) != '"') {
			// line breaks not allowed
			if (test == '\n') {
				throw File::Error("Line break in string");

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
				else
					throw File::Error("Unknown escape code");

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
		std::ostringstream data;
		// read in name
		data << (char)test;
		while (in && isdigit(in.peek()))
			data << (char)in.get();

		outstr = data.str();
		return TOKEN_NUMBER;

		// %begin
	} else if (test == '%') {
		std::ostringstream data;
		// read in name
		while (in) {
			test = in.peek();
			if (!isalnum(test) && test != '_')
				break;
			data << (char)in.get();
		}

		outstr = data.str();

		// begin?  we're a block
		if (outstr == "begin") {
			// must have whitespace to end of line
			while (in && in.peek() != '\n')
				if (!isspace(in.get()))
					throw File::Error("Syntax error: garbage after <<[");
			in.get();
			++line;

			// get block
			outstr.clear();
			while (in) {
				// read a line
				std::ostringstream data;
				do {
					char ch = in.get();
					data << ch;
					if (ch == '\n') {
						++ line;
						break;
					}
				} while (in);

				// line end pattern?
				std::string tstr = data.str();
				if (strip(tstr) == "%end") // see if the string exists, if so, see if that's all there is
					break;

				// add data
				outstr = outstr + tstr;
			}

			return TOKEN_STRING;
		} else {
			throw File::Error("Syntax error: unknown symbol: " + outstr);
		}

		// name
	} else if (isalpha(test) || test == '_') {
		std::ostringstream data;
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

		// we're a gneric name token
		return TOKEN_NAME;
	}

	// error
	std::ostringstream data;
	data << (char)test;
	while (in && !isspace(in.peek()))
		data << (char)in.get();

	throw File::Error("Syntax error: unknown symbol: " + data.str());
}

bool
File::Reader::get(Node& node)
{
	Token op;
	std::string opstr;
	std::vector<Value> list;

	// clear node
	node.ns.clear();
	node.name.clear();
	node.value = Value();
	node.type = Node::ERROR;

	// get op - expect name
	op = readToken(opstr);

	// end of file - ok
	if (op == TOKEN_EOF)
		return false;

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
		throw File::Error("File reader error: name expected");
	node.ns = opstr;

	// expect name separator
	op = readToken(opstr);
	if (op != TOKEN_KEY)
		throw File::Error("File reader error: expected .");

	// read name
	op = readToken(opstr);
	if (op != TOKEN_NAME && op != TOKEN_STRING)
		throw File::Error("File reader error: name expected after .");
	node.name = opstr;

	// object?
	op = readToken(opstr);
	if (op == TOKEN_BEGIN) {
		// return as object type
		node.type = Node::BEGIN_UNTYPED;
		return true;
	}

	// attribute?
	else if (op == TOKEN_SET) {
		// read
		std::string data;
		Token type = readToken(data);

		// attribute-object
		if (type == TOKEN_NAME) {
			node.value = Value(Value::TYPE_STRING, data);
			type = readToken(data);
			if (type != TOKEN_BEGIN)
				throw File::Error("File reader error: { expected after name");
			node.type = Node::BEGIN_TYPED;
			return true;

			// can we read the value?
		} else if (setValue(type, data, node.value)) {
			node.type = Node::ATTR;
			return true;

			// no value
		} else {
			// unknown type
			throw File::Error("File reader error: value expected after =");
		}

	} else {
		throw File::Error("File reader error: expected { or = after name");
	}
}

bool File::Reader::setValue(File::Reader::Token type, std::string& data, File::Value& value)
{
	if (type == TOKEN_NUMBER) {
		value = Value(Value::TYPE_INT, data);
	} else if (type == TOKEN_TRUE) {
		value = Value(Value::TYPE_BOOL, "true");
	} else if (type == TOKEN_FALSE) {
		value = Value(Value::TYPE_BOOL, "false");
	} else if (type == TOKEN_STRING) {
		value = Value(Value::TYPE_STRING, data);
	} else if (type == TOKEN_START_LIST) {
		std::vector<Value> list;
		while (true) {
			type = readToken(data);
			Value nvalue;
			if (type == TOKEN_START_LIST) {
				Log::Error << "Illegal to include a list in a list at " << getFilename() << ":" << getLine();
				return false;
			}
			if (!setValue(type, data, nvalue)) {
				Log::Error << "Unexpected token in list at " << getFilename() << ":" << getLine();
				return false;
			}
			list.push_back(nvalue);
			type = readToken(data);
			if (type == TOKEN_END_LIST)
				break;
			else if (type != TOKEN_COMMA) {
				Log::Error << "Unexpected token in list at " << getFilename() << ":" << getLine();
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
File::Reader::consume()
{
	Node node(*this);
	size_t depth = 0;

	// keep reading nodes until EOF or begin end
	while (get(node)) {
		// increment depth on new node
		if (node.isBegin())
			++depth;
		// decrement depth on end, and abort if we have depth 0 on end
		else if (node.isEnd())
			if (depth-- == 0)
				break;
	}
}

int
File::Writer::open(const std::string& filename)
{
	// open and write to a temp file
	path = filename;
	out.open((filename + "~").c_str());
	if (!out) {
		Log::Error << "Failed to open " << filename;
		return -1;
	}
	indent = 0;
	return 0;
}

void
File::Writer::close()
{
	if (out) {
		out << "# vim: set shiftwidth=2 tabstop=2 expandtab:\n";
		out.close();

		// move temp file to real file
		File::rename(path + "~", path);
	}
}

void
File::Writer::doIndent()
{
	for (unsigned long i = 0; i < indent; ++i)
		out << "  ";
}

void
File::Writer::attr(const std::string& ns, const std::string& name, const std::string& data)
{
	if (!out)
		return;

	if (!File::validName(ns) || !File::validName(name)) {
		Log::Error << "Attempted to write id '" << ns << "." << name << "'";
		return;
	}

	doIndent();
	out << ns << "." << name << " = " << EscapeString(data) << "\n";
}

void
File::Writer::attr(const std::string& ns, const std::string& name, long data)
{
	if (!out)
		return;

	if (!File::validName(ns) || !File::validName(name)) {
		Log::Error << "Attempted to write id '" << ns << "." << name << "'";
		return;
	}

	doIndent();
	out << ns << "." << name << " = " << data << "\n";
}

void
File::Writer::attr(const std::string& ns, const std::string& name, bool data)
{
	if (!out)
		return;

	if (!File::validName(ns) || !File::validName(name)) {
		Log::Error << "Attempted to write id '" << ns << "." << name << "'";
		return;
	}

	doIndent();
	out << ns << "." << name << " = " << (data ? "true" : "false") << "\n";
}

void
File::Writer::attr(const std::string& ns, const std::string& name, const std::vector<Value>& list)
{
	if (!out)
		return;

	if (!File::validName(ns) || !File::validName(name)) {
		Log::Error << "Attempted to write id '" << ns << "." << name << "'";
		return;
	}

	out << ns << "." << name << " = [ ";

	for (std::vector<Value>::const_iterator i = list.begin(); i != list.end(); ++i) {
		if (i != list.begin())
			out << ", ";
		switch (i->getType()) {
		case Value::TYPE_NONE:
		case Value::TYPE_LIST:
			out << "\"\"";
			break;
		case Value::TYPE_STRING:
			out << EscapeString(i->getValue());
			break;
		case Value::TYPE_INT:
		case Value::TYPE_BOOL:
			out << i->getValue();
			break;
		}
	}
	out << " ]\n";
}

void
File::Writer::block(const std::string& ns, const std::string& name, const std::string& data)
{
	if (!out)
		return;

	if (!File::validName(ns) || !File::validName(name)) {
		Log::Error << "Attempted to write id '" << ns << "." << name << "'";
		return;
	}

	doIndent();

	// beginning
	out << ns << "." << name << " = %begin\n" << data;
	// we need to add a newline if we don't have one on end already
	// FIXME: escape if data includes end line
	if (data.empty() || data[data.size()-1] != '\n')
		out << '\n';
	// ending
	doIndent();
	out << "%end\n";
}

void
File::Writer::begin(const std::string& ns, const std::string& name)
{
	if (!out)
		return;

	if (!File::validName(ns) || !File::validName(name)) {
		Log::Error << "Attempted to write id '" << ns << '.' << name << "'";
		return;
	}

	doIndent();
	out << ns << '.' << name << " {\n";
	++ indent;
}

void
File::Writer::beginAttr(const std::string& ns, const std::string& name, const std::string& type)
{
	if (!out)
		return;

	if (!File::validName(ns) || !File::validName(name)) {
		Log::Error << "Attempted to write id '" << ns << "." << name << "' {";
		return;
	}

	if (!File::validName(type)) {
		Log::Error << "Attempted to write type '" << type << "'";
		return;
	}

	doIndent();
	out << ns << '.' << name << " = " << type << " {\n";
	++ indent;
}

void
File::Writer::end()
{
	if (!out)
		return;

	// unindent, then write
	-- indent;
	doIndent();
	out << "}\n";
}

void
File::Writer::comment(const std::string& text)
{
	if (!out)
		return;
	if (text.empty())
		return;

	doIndent();

	// make sure we format comments with newlines properly
	out << "# ";
	const char* tptr = text.c_str();
	while (*tptr != '\0') {
		if (*tptr == '\n') {
			out << '\n';
			doIndent();
			out << "# ";
		} else {
			out << *tptr;
		}
		++ tptr;
	}
	out << '\n';
}

bool File::validName(const std::string& name)
{
	// not empty
	if (name.empty())
		return false;

	// must start with a letter or _
	if (!isalpha(name[0]) && name[0] != '_')
		return false;

	// must be alpha, digit, _
	for (std::string::const_iterator i = name.begin(); i != name.end(); ++i)
		if (!isalnum(*i) && *i != '_')
			return false;

	// all good
	return true;
}

int
File::Node::getInt() const
{
	if (value.getType() != Value::TYPE_INT) {
		Log::Error << "Incorrect data type for '" << getNs() << '.' << getName() << "' at :" << getLine();
		throw(File::Error("data type mismatch"));
	}

	return tolong(value.getValue());
}

bool
File::Node::getBool() const
{
	if (value.getType() != Value::TYPE_BOOL) {
		Log::Error << "Incorrect data type for '" << getNs() << '.' << getName() << "' at :" << getLine();
		throw(File::Error("data type mismatch"));
	}
	return value.getValue() == "true" || value.getValue() == "yes" || value.getValue() == "on";
}

std::string
File::Node::getString() const
{
	if (value.getType() != Value::TYPE_STRING) {
		Log::Error << "Incorrect data type for '" << getNs() << '.' << getName() << "' at :" << getLine();
		throw(File::Error("data type mismatch"));
	}
	return value.getValue();
}

const std::vector<File::Value>&
File::Node::getList() const
{
	if (value.getType() != Value::TYPE_LIST) {
		Log::Error << "Incorrect data type for '" << getNs() << '.' << getName() << "' at :" << getLine();
		throw(File::Error("data type mismatch"));
	}
	return value.getList();
}

const std::vector<File::Value>&
File::Node::getList(size_t size) const
{
	if (value.getType() != Value::TYPE_LIST) {
		Log::Error << "Incorrect data type for '" << getNs() << '.' << getName() << "' at :" << getLine();
		throw(File::Error("data type mismatch"));
	}
	if (value.getList().size() != size) {
		Log::Error << "Incorrect number of elements in list for '" << getNs() << '.' << getName() << "' at :" << getLine();
		throw(File::Error("list size mismatch"));
	}
	return value.getList();
}

std::string
File::Node::getString(size_t index) const
{
	if (value.getType() != Value::TYPE_LIST) {
		Log::Error << "Incorrect data type for '" << getNs() << '.' << getName() << "' at :" << getLine();
		throw(File::Error("data type mismatch"));
	}
	if (value.getList().size() <= index) {
		Log::Error << "Incorrect number of elements in list for '" << getNs() << '.' << getName() << "' at :" << getLine();
		throw(File::Error("list size mismatch"));
	}
	if (value.getList()[index].getType() != Value::TYPE_STRING) {
		Log::Error << "Incorrect data type for element " << (index + 1) << " of '" << getNs() << '.' << getName() << "' at :" << getLine();
		throw(File::Error("data type mismatch"));
	}
	return value.getList()[index].getValue();
}

int
File::Node::getInt(size_t index) const
{
	if (value.getType() != Value::TYPE_LIST) {
		Log::Error << "Incorrect data type for '" << getNs() << '.' << getName() << "' at :" << getLine();
		throw(File::Error("data type mismatch"));
	}
	if (value.getList().size() <= index) {
		Log::Error << "Incorrect number of elements in list for '" << getNs() << '.' << getName() << "' at :" << getLine();
		throw(File::Error("list size mismatch"));
	}
	if (value.getList()[index].getType() != Value::TYPE_INT) {
		Log::Error << "Incorrect data type for element " << (index + 1) << " of '" << getNs() << '.' << getName() << "' at :" << getLine();
		throw(File::Error("data type mismatch"));
	}
	return tolong(value.getList()[index].getValue());
}

const StreamControl&
operator<< (const StreamControl& stream, const File::Node& node)
{
	stream << node.getReader().getPath() << ',' << node.getLine() << ": ";
	switch (node.getNodeType()) {
	case File::Node::ATTR:
		stream << node.getNs() << '.' << node.getName() << " (";
		switch (node.getValueType()) {
		case File::Value::TYPE_NONE:
			stream << "nil";
			break;
		case File::Value::TYPE_STRING:
			stream << "string";
			break;
		case File::Value::TYPE_INT:
			stream << "int";
			break;
		case File::Value::TYPE_BOOL:
			stream << (node.getBool() ? "true" : "false");
			break;
		case File::Value::TYPE_LIST:
			stream << "list";
			break;
		default:
			stream << "unknown";
			break;
		}
		stream << ')';
		break;
	case File::Node::BEGIN_TYPED:
		stream << node.getNs() << '.' << node.getName() << " (" << node.getString() << ')';
		break;
	case File::Node::BEGIN_UNTYPED:
		stream << node.getNs() << '.' << node.getName();
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
