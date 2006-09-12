/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "common/streams.h"
#include "mud/parse.h"

StreamParse::StreamParse(String s_text)
	: text(s_text), argv()
{}

StreamParse::StreamParse(String s_text, String s_name, ParseValue s_value)
	: text(s_text)
{
	argv[s_name] = s_value;
}

StreamParse::StreamParse(String s_text, String s_name1, ParseValue s_value1, String s_name2, ParseValue s_value2)
	: text(s_text)
{
	argv[s_name1] = s_value1;
	argv[s_name2] = s_value2;
}

StreamParse::StreamParse(String s_text, String s_name1, ParseValue s_value1, String s_name2, ParseValue s_value2, String s_name3, ParseValue s_value3)
	: text(s_text)
{
	argv[s_name1] = s_value1;
	argv[s_name2] = s_value2;
	argv[s_name3] = s_value3;
}

StreamParse&
StreamParse::add (String s_name, ParseValue s_value)
{
	argv[s_name] = s_value;
	return *this;
}
