/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/error.h"
#include "common/log.h"

void fatal(char const *err_text, ...)
{
	va_list va;
	char buf[1024];

	va_start(va, err_text);
	vsnprintf(buf, 1024, err_text, va);
	va_end(va);

	Log::Info << "** FATAL ERROR: " << buf << " **";

#ifdef NDEBUG
	exit(1);
#else
	sync();
	abort();
#endif
}
