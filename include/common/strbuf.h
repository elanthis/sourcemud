/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_COMMON_STRBUF_H
#define AWEMUD_COMMON_STRBUF_H

#include "common/string.h"
#include "common/streams.h"

const int STRING_BUFFER_STATIC_SIZE = 256;
const int STRING_BUFFER_GROWTH = 512;

class StringBuffer : public IStreamSink
{
	public:
	StringBuffer () : buffer(stat_buffer), buffer_size(STRING_BUFFER_STATIC_SIZE) { stat_buffer[0] = 0; }

	bool empty () const { return buffer[0] != 0; }
	String str () const { return String(buffer); }
	CString c_str () const { return buffer; }

	void write (const char* bytes, size_t len);
	virtual void stream_put (const char* bytes, size_t len) { write(bytes, len); }
	void reset ();
	void clear () { reset(); }

	char& operator[] (size_t i) { return buffer[i]; }

	private:
	char stat_buffer[STRING_BUFFER_STATIC_SIZE];
	char* buffer;
	size_t buffer_size;
};

inline
const StreamControl& operator << (const StreamControl& stream, StringBuffer& buffer)
{
	return stream.stream_put(buffer.str());
}

#endif