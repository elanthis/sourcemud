/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_TIME_H
#define SOURCEMUD_COMMON_TIME_H

#include <time.h>

#include "common/string.h"

String time_to_str (time_t time);
time_t str_to_time (String str);

#endif
