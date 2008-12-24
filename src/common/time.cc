/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/time.h"

const std::string Time::RFC_822_FORMAT = "%a, %d %b %Y %H:%M:%S %z";

std::string time_to_str(time_t time)
{
	struct tm tm;
	char buffer[128];

	localtime_r(&time, &tm);
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
	return std::string(buffer);
}

time_t str_to_time(const std::string& str)
{
	struct tm tm;
	memset(&tm, 0, sizeof(tm));
	strptime(str.c_str(), "%Y-%m-%d %H:%M:%S", &tm);
	return mktime(&tm);
}

std::string Time::format(const std::string& format, time_t time)
{
	char buf[64];
	strftime(buf, sizeof(buf), format.c_str(), localtime(&time));
	return buf;
}
