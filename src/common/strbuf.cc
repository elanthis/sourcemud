/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "common/strbuf.h"
#include "common/log.h"

void StringBuffer::reset ()
{
	if (buffer != stat_buffer)
		GC_FREE(buffer);
	buffer = stat_buffer;
	buffer_size = STRING_BUFFER_STATIC_SIZE;
	buffer[0] = 0;
}

void StringBuffer::write (const char* bytes, size_t len)
{
	size_t cur_len = strlen(buffer);

	if (cur_len + len + 1 > buffer_size) {
		char* new_buf = (char*)GC_MALLOC(buffer_size + STRING_BUFFER_GROWTH);
		memcpy(new_buf, buffer, cur_len);
		if (buffer != stat_buffer)
			GC_FREE(buffer);
		buffer = new_buf;
		buffer_size += STRING_BUFFER_GROWTH;
	}
	memcpy(buffer + cur_len, bytes, len);
	buffer[cur_len + len] = 0;
}
