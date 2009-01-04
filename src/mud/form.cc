/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/string.h"
#include "mud/form.h"

std::vector<std::string> FormColor::names;

std::string FormBuild::names[] = {
	"none",
	"hunched",
	"gaunt",
	"lean",
	"average",
	"husky",
	"stocky",
	"powerful",
	"athletic",
};

std::string FormHeight::names[] = {
	"none",
	"tiny",
	"short",
	"typical",
	"tall",
};

std::string FormHairStyle::names [] = {
	"none",
	"long, straight",
	"long, wavy",
	"long, curly",
	"short, straight",
	"short, wavy",
	"short, curly",
};

std::string FormColor::get_name() const
{
	if (!valid())
		return std::string();
	else
		return names[value-1];
}

FormColor FormColor::lookup(const std::string& name)
{
	for (size_t i = 0; i < names.size(); ++i) {
		if (names[i] == name)
			return FormColor(i + 1);
	}
	return FormColor();
}

FormColor FormColor::create(const std::string& name)
{
	for (size_t i = 0; i < names.size(); ++i) {
		if (names[i] == name)
			return FormColor(i + 1);
	}
	names.push_back(name);
	return FormColor(names.size());
}

FormBuild FormBuild::lookup(const std::string& name)
{
	for (size_t i = 0; i < COUNT; ++i) {
		if (names[i] == name)
			return FormBuild(i);
	}
	return FormBuild();
}

FormHeight FormHeight::lookup(const std::string& name)
{
	for (size_t i = 0; i < COUNT; ++i) {
		if (names[i] == name)
			return FormHeight(i);
	}
	return FormHeight();
}

FormHairStyle FormHairStyle::lookup(const std::string& name)
{
	for (size_t i = 0; i < COUNT; ++i) {
		if (names[i] == name)
			return FormHairStyle(i);
	}
	return FormHairStyle();
}
