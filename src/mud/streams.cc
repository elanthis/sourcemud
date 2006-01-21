/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "common/streams.h"
#include "mud/parse.h"
#include "mud/room.h"

// flush room output
bool
RoomStreamSink::stream_end (void) {
	// send output
	if (buffer) {
		if (ignores.empty())
			room.put(buffer, buffer.size());
		else
			room.put(buffer, buffer.size(), &ignores);
		buffer.clear();
	}
	// yes, delete this sink
	return true;
}

StreamParse::StreamParse(StringArg s_text)
	: text(s_text), argv(), names()
{}

StreamParse::StreamParse(StringArg s_text, StringArg s_name, ParseValue s_value)
	: text(s_text), argv(1), names(1)
{
	names[0] = s_name;
	argv[0] = s_value;
}

StreamParse::StreamParse(StringArg s_text, StringArg s_name1, ParseValue s_value1, StringArg s_name2, ParseValue s_value2)
	: text(s_text), argv(2), names(2)
{
	names[0] = s_name1;
	argv[0] = s_value1;
	names[1] = s_name2;
	argv[1] = s_value2;
}

StreamParse::StreamParse(StringArg s_text, StringArg s_name1, ParseValue s_value1, StringArg s_name2, ParseValue s_value2, StringArg s_name3, ParseValue s_value3)
	: text(s_text), argv(3), names(3)
{
	names[0] = s_name1;
	argv[0] = s_value1;
	names[1] = s_name2;
	argv[1] = s_value2;
	names[2] = s_name3;
	argv[2] = s_value3;
}

StreamParse&
StreamParse::add (StringArg s_name, ParseValue s_value)
{
	names.push_back(s_name);
	argv.push_back(s_value);
	return *this;
}

StreamParse&
StreamParse::add (ParseValue s_value)
{
	names.push_back(String());
	argv.push_back(s_value);
	return *this;
}
