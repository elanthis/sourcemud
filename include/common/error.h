/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_ERROR_H
#define SOURCEMUD_ERROR_H

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

#endif // SOURCEMUD_ERROR_H
