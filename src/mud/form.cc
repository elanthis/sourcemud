/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common/gcmap.h"
#include "mud/form.h"
#include "common/string.h"

StringList FormColor::names;

String FormBuild::names[] = {
	S("none"),
	S("hunched"),
	S("gaunt"),
	S("lean"),
	S("average"),
	S("husky"),
	S("stocky"),
	S("powerful"),
	S("athletic"),
};

String FormHeight::names[] = {
	S("none"),
	S("tiny"),
	S("short"),
	S("typical"),
	S("tall"),
};

String FormHairStyle::names [] = {
	S("none"),
	S("long, straight"),
	S("long, wavy"),
	S("long, curly"),
	S("short, straight"),
	S("short, wavy"),
	S("short, curly"),
};

String
FormColor::get_name () const
{
	if (!valid())
		return String();
	else
		return names[value-1];
}

FormColor
FormColor::lookup (String name)
{
	for (size_t i = 0; i < names.size(); ++i) {
		if (names[i] == name)
			return FormColor(i+1);
	}
	return FormColor();
}

FormColor
FormColor::create (String name)
{
	for (size_t i = 0; i < names.size(); ++i) {
		if (names[i] == name)
			return FormColor(i+1);
	}
	names.push_back(name);
	return FormColor(names.size());
}

FormBuild
FormBuild::lookup (String name)
{
	for (size_t i = 0; i < COUNT; ++i) {
		if (names[i] == name)
			return FormBuild(i);
	}
	return FormBuild();
}

FormHeight
FormHeight::lookup (String name)
{
	for (size_t i = 0; i < COUNT; ++i) {
		if (names[i] == name)
			return FormHeight(i);
	}
	return FormHeight();
}

FormHairStyle
FormHairStyle::lookup (String name)
{
	for (size_t i = 0; i < COUNT; ++i) {
		if (names[i] == name)
			return FormHairStyle(i);
	}
	return FormHairStyle();
}
