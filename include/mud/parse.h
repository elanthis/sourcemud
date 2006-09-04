/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef PARSE_H
#define PARSE_H

#include "common/gcvector.h"
#include "common/streams.h"

class ParseValue;

typedef StringList ParseNames;
typedef GCType::vector<ParseValue> ParseArgs;

class Parsable
{
	public:
	virtual ~Parsable () {}

	// return non-zero if the requested method/property does not exist
	virtual int parse_property (const class StreamControl& stream, String method, const ParseArgs& argv) const = 0;

	// stream a default desc/name/whatever
	virtual void parse_default (const class StreamControl& stream) const = 0;
};

class ParseValue : public GC
{
	public:
	enum Type { T_OBJECT, T_STRING, T_NULL };

	// constructors
	inline ParseValue () : type(T_NULL), object(NULL), string() {}
	inline ParseValue (const Parsable* s_object) : type(s_object == NULL ? T_NULL : T_OBJECT), object(s_object), string() {}
	inline ParseValue (String s_string) : type(T_STRING), object(NULL), string(s_string) {}

	// fetch the type of the mixed value
	inline Type get_type () const { return type; }
	inline bool is_object () const { return type == T_OBJECT; }
	inline bool is_string () const { return type == T_STRING; }
	inline bool is_null () const { return type == T_NULL; }

	// specific getters
	inline const Parsable* get_object () const { return this->object; }
	inline const String& get_string () const { return this->string; }

	// assign
	inline const Parsable* operator= (const Parsable* object) { type = (object == NULL ? T_NULL : T_OBJECT); return this->object = object; }
	inline const String& operator= (String string) { type = T_STRING; return this->string = string; }

	private:
	Type type;
	const Parsable* object;
	String string;
};

namespace parse {
	const StreamControl& text (const StreamControl& stream, String format, const ParseArgs& argv, const ParseNames& names);
}

// parse info
struct StreamParse {
	StreamParse(String s_text);
	StreamParse(String s_text, String s_name, ParseValue s_value);
	StreamParse(String s_text, String s_name1, ParseValue s_value1, String s_name2, ParseValue s_value2);
	StreamParse(String s_text, String s_name1, ParseValue s_value1, String s_name2, ParseValue s_value2, String s_name3, ParseValue s_value3);

	StreamParse& add (String s_name, ParseValue s_value);
	StreamParse& add (ParseValue s_value);

	friend inline
	const StreamControl&
	operator << (const StreamControl& stream, const StreamParse& sparse)
	{
		return parse::text(stream, sparse.text, sparse.argv, sparse.names);
	}

	private:
	String text;
	ParseArgs argv;
	ParseNames names;
};

#endif
