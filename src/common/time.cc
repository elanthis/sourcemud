/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "common/time.h"

String
time_to_str (time_t time)
{
	struct tm tm;
	char buffer[128];

	localtime_r(&time, &tm);
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %Z", &tm);
	return String(buffer);
}

time_t
str_to_time (String str)
{
	struct tm tm;
	strptime(str.c_str(), "%Y-%m-%d %H:%M:%S %Z", &tm);
	return mktime(&tm);
}
