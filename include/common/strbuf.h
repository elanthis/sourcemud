/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_STRBUF_H
#define SOURCEMUD_COMMON_STRBUF_H

#include "common/string.h"
#include "common/streams.h"

const int STRING_BUFFER_STATIC_SIZE = 256;
const int STRING_BUFFER_GROWTH = 512;

class StringBuffer : public IStreamSink
{
	public:
	StringBuffer() : buffer(stat_buffer), buffer_size(STRING_BUFFER_STATIC_SIZE) { stat_buffer[0] = 0; }
	~StringBuffer();

	bool empty () const { return buffer[0] == 0; }
	std::string str () const { return std::string(buffer); }
	const char* c_str () const { return buffer; }

	size_t size () const { return strlen(buffer); }

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
	return stream.stream_put(buffer.str().c_str());
}

#endif
