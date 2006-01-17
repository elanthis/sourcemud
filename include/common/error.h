/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_ERROR_H
#define AWEMUD_ERROR_H

#include <errno.h>

void fatal (char const *, ...);		// server will immediately die

#undef assert
#ifndef NDEBUG
#define assert(exp) \
	do { \
		if (!(exp)) { \
			fatal( "File: %s, Line %d (%s): Assertain failed (%s)", \
				__FILE__, \
				__LINE__, \
				__func__, \
				#exp \
			); \
		} \
	} while(0)
#else // NDEBUG
#define assert(exp)
#endif // NDEBUG

#endif // AWEMUD_ERROR_H
