/*
 * Source MUD
 * Copyright(C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_STREAMTIME_H
#define SOURCEMUD_COMMON_STREAMTIME_H 1

#include "common/streams.h"
#include "common/time.h"

class StreamTime
{
public:
	explicit StreamTime(const char* s_format, time_t s_time) : format(s_format), time(s_time) {}
	explicit StreamTime(const char* s_format) : format(s_format) { ::time(&time); }
	StreamTime() : format(Time::RFC_822_FORMAT.c_str()) { ::time(&time); }

	const char* get_format() const { return format; }
	const time_t& get_time() const { return time; }

private:
	const char* format;
	time_t time;
};

const StreamControl& operator<<(const StreamControl& stream, const StreamTime& time)
{
	char buf[64];
	strftime(buf, sizeof(buf), time.get_format(), localtime(&time.get_time()));
	stream << buf;
	return stream;
}

#endif
