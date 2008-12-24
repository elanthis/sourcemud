/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/log.h"

int
fdprintf (int fd, const char* format, ...)
{
	char buffer[2048];
	char* ptr;
	va_list va;
	int ret, bytes, amount;

	// format text
	va_start(va, format);
	bytes = vsnprintf(buffer, sizeof(buffer), format, va);
	va_end(va);

	// bail if there's 0 bytes OR an error
	if (bytes <= 0)
		return bytes;

	// log error on overflow
	if (bytes >= (int)sizeof(buffer))
		Log::Error << "fdprintf(): overflow [" << bytes << "]";

	// find actual length of buffer; might have over-flowed
	bytes = strlen(buffer);

	// keep writing out to descriptor until we are out of bytes or hit an error
	ptr = buffer;
	amount = 0;
	while (amount < bytes) {
		// write, bail on error
		if ((ret = write(fd, ptr, bytes - amount)) == -1 && errno != EAGAIN)
			return -1;

		// modify ptr/bytes
		if (ret != -1) {
			amount += ret;
			ptr += ret;
		}
	}

	return bytes;
}
