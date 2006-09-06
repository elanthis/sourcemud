/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_PARSE_H
#define AWEMUD_MUD_PARSE_H

#include "common/gcmap.h"
#include "common/gcvector.h"
#include "common/streams.h"

class ParseValue;

typedef GCType::map<String, ParseValue> ParseArgs;
typedef GCType::vector<ParseValue> ParseList;

class Parsable
{
	public:
	virtual ~Parsable () {}

	// return non-zero if the requested method/property does not exist
	virtual int parse_property (const class StreamControl& stream, String method, const ParseList& argv) const = 0;

	// stream a default desc/name/whatever
	virtual void parse_default (const class StreamControl& stream) const = 0;
};

class ParseValue : public GC
{
	public:
	enum Type { T_OBJECT, T_STRING, T_NULL };

	// constructors
	ParseValue () : type(T_NULL), object(NULL), string() {}
	ParseValue (const Parsable* s_object) : type(s_object == NULL ? T_NULL : T_OBJECT), object(s_object), string() {}
	ParseValue (String s_string) : type(T_STRING), object(NULL), string(s_string) {}

	// fetch the type of the mixed value
	Type get_type () const { return type; }
	bool is_object () const { return type == T_OBJECT; }
	bool is_string () const { return type == T_STRING; }
	bool is_null () const { return type == T_NULL; }

	// specific getters
	const Parsable* get_object () const { return this->object; }
	const String& get_string () const { return this->string; }

	// assign
	const Parsable* operator= (const Parsable* object) { type = (object == NULL ? T_NULL : T_OBJECT); return this->object = object; }
	const String& operator= (String string) { type = T_STRING; return this->string = string; }
	ParseValue& operator= (const ParseValue& base) { type = base.type; object = base.object; string = base.string; return *this; }

	private:
	Type type;
	const Parsable* object;
	String string;
};

namespace parse {
	const StreamControl& text (const StreamControl& stream, String format, const ParseArgs& argv);
}

// parse info
struct StreamParse {
	StreamParse(String s_text);
	StreamParse(String s_text, String s_name, ParseValue s_value);
	StreamParse(String s_text, String s_name1, ParseValue s_value1, String s_name2, ParseValue s_value2);
	StreamParse(String s_text, String s_name1, ParseValue s_value1, String s_name2, ParseValue s_value2, String s_name3, ParseValue s_value3);

	StreamParse& add (String s_name, ParseValue s_value);

	friend inline
	const StreamControl&
	operator << (const StreamControl& stream, const StreamParse& sparse)
	{
		return parse::text(stream, sparse.text, sparse.argv);
	}

	private:
	String text;
	ParseArgs argv;
};

#endif
