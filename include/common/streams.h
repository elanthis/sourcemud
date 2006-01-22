/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_STREAMS_H
#define AWEMUD_STREAMS_H

#include "common/gcbase.h"
#include "common/string.h"

// pre-defines
class Character;
class Room;
class StreamControl;

// base stream type
class IStreamSink : public GC {
	public:
	virtual ~IStreamSink (void) {}

	virtual void stream_put (const char* text, size_t len = 0) = 0;
	inline virtual void stream_ignore (class Character* ch) {}
	inline virtual bool stream_end (void) { return false; }
};

// String stream
class
StringStreamSink : public IStreamSink {
	public:
	StringStreamSink (String& s_string) : string(s_string) {}

	// append to the string
	inline virtual
	void
	stream_put (const char* text, size_t len) {
		string.append(text, len);
	}

	// yes, delete the sink
	inline virtual
	bool
	stream_end (void) {
		return true;
	}

	private:
	String& string;
};

// string buffer sink
template <size_t Size>
class
BufferSink : public IStreamSink {
	public:
	inline BufferSink (void) : ptr(buffer) { buffer[0] = 0; }

	// write to the stream
	virtual
	void
	stream_put (const char* text, size_t len)
	{
		// trim length
		if (len >= Size - (ptr - buffer))
			len = Size - (ptr - buffer) - 1;

		// append
		strncpy(ptr, text, len);
		ptr[len] = 0;

		// ready for next one
		ptr += len;
	}

	// get the string
	inline const char* c_str (void) const { return buffer; }
	inline char* c_str (void) { return buffer; }
	inline String str (void) { return buffer; }
	inline operator const char* (void) const { return c_str(); }
	inline operator char* (void) { return c_str(); }
	inline bool empty (void) const { return buffer == ptr; }
	inline size_t size (void) const { return ptr - buffer; }

	// misc
	inline void clear (void) { buffer[0] = 0; ptr = buffer; }

	private:
	char buffer[Size];
	char* ptr;
};

// Room stream
class
RoomStreamSink : public IStreamSink {
	public:
	RoomStreamSink (class Room& s_room) : room(s_room), buffer(), ignores() {}

	// send out the text
	inline virtual
	void
	stream_put (const char* text, size_t len) {
		buffer.append(text, len);
	}

	// add an ignore
	inline virtual
	void
	stream_ignore (class Character* ch) {
		ignores.push_back(ch);
	}

	// flush output
	virtual bool stream_end (void);

	private:
	class Room& room;
	String buffer;
	typedef GCType::vector<class Character*> IgnoreList;
	IgnoreList ignores;
};

// base awemud stream type
class
StreamControl {
	public:
	// create sink
	inline StreamControl (IStreamSink* sptr) : sink(sptr) {}
	inline StreamControl (IStreamSink& sref) : sink(&sref) {}
	inline StreamControl (class Room& rref) : sink(new RoomStreamSink(rref)) {}
	inline StreamControl (String& sref) : sink(new StringStreamSink(sref)) {}

	// finish stream
	inline
	~StreamControl (void) {
		// delete sink if stream_end returns true
		if (sink->stream_end())
			delete sink;
	}

	// send text
	inline
	const StreamControl&
	stream_put (const char* text, size_t len = 0) const {
		sink->stream_put(text, len ? len : strlen(text));
		return *this;
	}

	// basically just for rooms - yes, an evil hack
	inline
	const StreamControl&
	stream_ignore (class Character* ch) const {
		sink->stream_ignore(ch);
		return *this;
	}

	private:
	IStreamSink* sink;
};

// ignore holder
struct StreamIgnore {
	explicit StreamIgnore(class Character* s_ch) : ch(s_ch) {}

	friend inline
	const StreamControl&
	operator << (const StreamControl& stream, const StreamIgnore& ignore)
	{
		return stream.stream_ignore (ignore.ch);
	}

	class Character* ch;
};

// stream a chunk of a string
struct StreamChunk {
	explicit StreamChunk(const char* s_text, size_t s_len) : text(s_text), len(s_len) {}
	explicit StreamChunk(const char* s_text, const char* s_last) : text(s_text), len(s_last-s_text+1) {}

	friend inline
	const StreamControl&
	operator << (const StreamControl& stream, const StreamChunk& chunk)
	{
		return stream.stream_put(chunk.text, chunk.len);
	}

	const char* text;
	size_t len;
};

// C++ style stream operators
inline
const StreamControl& operator << (const StreamControl& stream, long i)
{
	char buf[40];
	snprintf(buf,sizeof(buf),"%ld",i);
	return stream.stream_put(buf);
}
inline
const StreamControl& operator << (const StreamControl& stream, unsigned long ui)
{
	char buf[40];
	snprintf(buf,sizeof(buf),"%lu",ui);
	return stream.stream_put(buf);
}
inline
const StreamControl& operator << (const StreamControl& stream, int i)
{
	return stream << (long)i;
}
inline
const StreamControl& operator << (const StreamControl& stream, unsigned int ui)
{
	return stream << (unsigned long)ui;
}
inline
const StreamControl& operator << (const StreamControl& stream, const void* ptr)
{
	char buf[40];
	snprintf(buf,sizeof(buf),"%p",ptr);
	return stream.stream_put(buf);
}
inline
const StreamControl& operator << (const StreamControl& stream, const char *cstr)
{
	return stream.stream_put (cstr);
}
inline
const StreamControl& operator << (const StreamControl& stream, const String& string)
{
	return stream.stream_put (string.c_str());
}
inline
const StreamControl& operator << (const StreamControl& stream, const char ch)
{
	const char buf[2] = {ch, '\0'};
	return stream.stream_put(buf, 1);
}
template <size_t Size>
inline
const StreamControl& operator << (const StreamControl& stream, const BufferSink<Size>& buffer)
{
	stream.stream_put(buffer.c_str(), buffer.size());
	return stream;
}

#endif // AWEMUD_STREAMS
