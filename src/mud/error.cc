/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

#include "error.h"
#include "log.h"

void
fatal (char const *err_text, ...)
{
	va_list va;
	char buf[1024];

	va_start (va, err_text);
	vsnprintf (buf, 1024, err_text, va);
	va_end (va);

	Log::Info << "** FATAL ERROR: " << buf << " **";

#ifdef NDEBUG
	exit (1);
#else
	sync();
	abort();
#endif
}
