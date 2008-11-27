/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common/streams.h"
#include "mud/macro.h"

StreamMacro::StreamMacro(std::string s_text)
	: text(s_text), argv()
{}

StreamMacro::StreamMacro(std::string s_text, std::string s_name, MacroValue s_value)
	: text(s_text)
{
	argv[s_name] = s_value;
}

StreamMacro::StreamMacro(std::string s_text, std::string s_name1, MacroValue s_value1, std::string s_name2, MacroValue s_value2)
	: text(s_text)
{
	argv[s_name1] = s_value1;
	argv[s_name2] = s_value2;
}

StreamMacro::StreamMacro(std::string s_text, std::string s_name1, MacroValue s_value1, std::string s_name2, MacroValue s_value2, std::string s_name3, MacroValue s_value3)
	: text(s_text)
{
	argv[s_name1] = s_value1;
	argv[s_name2] = s_value2;
	argv[s_name3] = s_value3;
}

StreamMacro&
StreamMacro::add (std::string s_name, MacroValue s_value)
{
	argv[s_name] = s_value;
	return *this;
}
