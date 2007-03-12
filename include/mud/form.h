/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_FORM_H
#define AWEMUD_MUD_FORM_H

#include "common/string.h"
#include "common/types.h"
#include "common/gcvector.h"

class FormColor {
	public:
	explicit FormColor (uint8_t s_value) : value(s_value) {}
	FormColor () : value(0) {}
	FormColor (const FormColor& c) : value(c.value) {}

	String get_name () const;

	uint8_t get_value () const { return value; }

	static FormColor lookup (String);
	static FormColor create (String);

	bool valid () const { return value > 0 && value < names.size(); }

	bool operator == (const FormColor& c) const { return value == c.value; }
	bool operator < (const FormColor& c) const { return value < c.value; }

	private:
	static StringList names;
	uint8_t value;
};

class FormBuild {
	public:
	typedef enum {
		NONE = 0,
		HUNCHED,
		GAUNT,
		LEAN,
		AVERAGE,
		HUSKY,
		STOCKY,
		POWERFUL,
		ATHLETIC,
		COUNT,
	} type_t;
	
	explicit FormBuild (int s_value) : value((type_t)s_value) {}
	FormBuild (type_t s_value) : value(s_value) {}
	FormBuild (const FormBuild& form) : value(form.value) {}
	FormBuild () : value(NONE) {}

	String get_name () const { return valid()?names[value]:String(); }

	type_t get_value () const { return value; }

	static FormBuild lookup (String name);

	bool valid () const { return value > 0 && value < COUNT; }

	bool operator == (FormBuild dir) const { return dir.value == value; }
	bool operator < (FormBuild dir) const { return value < dir.value; }

	private:
	type_t value;

	static String names[];
};

class FormHeight {
	public:
	typedef enum {
		NONE = 0,
		TINY,
		SHORT,
		TYPICAL,
		TALL,
		COUNT,
	} type_t;
	
	explicit FormHeight (int s_value) : value((type_t)s_value) {}
	FormHeight (type_t s_value) : value(s_value) {}
	FormHeight (const FormHeight& form) : value(form.value) {}
	FormHeight () : value(NONE) {}

	String get_name () const { return valid()?names[value]:String(); }

	type_t get_value () const { return value; }

	static FormHeight lookup (String name);

	bool valid () const { return value > 0 && value < COUNT; }

	bool operator == (FormHeight dir) const { return dir.value == value; }
	bool operator < (FormHeight dir) const { return value < dir.value; }

	private:
	type_t value;

	static String names[];
};

class FormHairStyle {
	public:
	typedef enum {
		NONE = 0,
		LONG_STRAIGHT,
		LONG_WAVY,
		LONG_CURLY,
		SHORT_STRAIGHT,
		SHORT_WAVY,
		SHORT_CURLY,
		COUNT,
	} type_t;
	
	explicit FormHairStyle (int s_value) : value((type_t)s_value) {}
	FormHairStyle (type_t s_value) : value(s_value) {}
	FormHairStyle (const FormHairStyle& form) : value(form.value) {}
	FormHairStyle () : value(NONE) {}

	String get_name () const { return valid()?names[value]:String(); }

	type_t get_value () const { return value; }

	static FormHairStyle lookup (String name);

	bool valid () const { return value > 0 && value < COUNT; }

	bool operator == (FormHairStyle dir) const { return dir.value == value; }
	bool operator < (FormHairStyle dir) const { return value < dir.value; }

	private:
	type_t value;

	static String names[];
};

#endif
