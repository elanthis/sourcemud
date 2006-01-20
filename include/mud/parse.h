/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef PARSE_H
#define PARSE_H

#include "gcvector.h"
#include "streams.h"

class Entity;
class Character;

class ParseValue : public GC
{
	public:
	enum Type { T_ENTITY, T_STRING, T_NULL };

	// constructors
	inline ParseValue () : type(T_NULL), entity(NULL), string() {}
	inline ParseValue (const Entity* s_entity) : type(s_entity == NULL ? T_NULL : T_ENTITY), entity(s_entity), string() {}
	inline ParseValue (StringArg s_string) : type(T_STRING), entity(NULL), string(s_string) {}

	// fetch the type of the mixed value
	inline Type get_type () const { return type; }
	inline bool is_entity () const { return type == T_ENTITY; }
	inline bool is_string () const { return type == T_STRING; }
	inline bool is_null () const { return type == T_NULL; }

	// specific getters
	inline const Entity* get_entity () const { return this->entity; }
	inline const String& get_string () const { return this->string; }

	// assign
	inline const Entity* operator= (const Entity* entity) { type = (entity == NULL ? T_NULL : T_ENTITY); return this->entity = entity; }
	inline const String& operator= (StringArg string) { type = T_STRING; return this->string = string; }

	private:
	Type type;
	const Entity* entity;
	String string;
};

typedef StringList ParseNames;
typedef GCType::vector<ParseValue> ParseArgs;

namespace parse {
	const StreamControl& text (const StreamControl& stream, StringArg format, const ParseArgs& argv, const ParseNames& names);
}

// parse info
struct StreamParse {
	StreamParse(StringArg s_text);
	StreamParse(StringArg s_text, StringArg s_name, ParseValue s_value);
	StreamParse(StringArg s_text, StringArg s_name1, ParseValue s_value1, StringArg s_name2, ParseValue s_value2);
	StreamParse(StringArg s_text, StringArg s_name1, ParseValue s_value1, StringArg s_name2, ParseValue s_value2, StringArg s_name3, ParseValue s_value3);

	StreamParse& add (StringArg s_name, ParseValue s_value);
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
