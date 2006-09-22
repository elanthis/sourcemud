/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_COMMON_TIME_H
#define AWEMUD_COMMON_TIME_H

#include <time.h>

#include "common/string.h"

String time_to_str (time_t time);
time_t str_to_time (String str);

#endif
