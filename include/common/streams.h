/*
 * Source MUD
 * Copyright(C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_STREAMS_H
#define SOURCEMUD_COMMON_STREAMS_H 1

// pre-defines
class Creature;
class Room;
class StreamControl;

// base stream type
class IStreamSink {
	public:
	virtual ~IStreamSink() {}

	virtual void stream_put(const char* text, size_t len = 0) = 0;
	inline virtual void stream_ignore(class Creature* ch) {}
	inline virtual void stream_end() {}
};

// base stream type
class
StreamControl {
	public:
	// create sink
	inline StreamControl(IStreamSink* sptr) : sink(sptr) { assert(sink != NULL); }
	inline StreamControl(IStreamSink& sref) : sink(&sref) { assert(sink != NULL); }
	StreamControl(class Room& rref); // this is in src/mud/room.cc; HACK

	// automatically finish stream
	~StreamControl() { sink->stream_end(); }

	// send text
	const StreamControl& stream_put(const char* text, size_t len) const {
		sink->stream_put(text, len);
		return *this;
	}
	const StreamControl& stream_put(const char* text) const {
		stream_put(text, strlen(text));
		return *this;
	}
	const StreamControl& stream_put(const std::string& text) const {
		return stream_put(text.c_str());
	}

	// basically just for rooms - yes, an evil hack
	const StreamControl& stream_ignore(class Creature* ch) const {
		sink->stream_ignore(ch);
		return *this;
	}

	private:
	IStreamSink* sink;
};

// ignore holder
struct StreamIgnore {
	explicit StreamIgnore(class Creature* s_ch) : ch(s_ch) {}

	friend const StreamControl& operator <<(const StreamControl& stream, const StreamIgnore& ignore)
	{
		return stream.stream_ignore(ignore.ch);
	}

	class Creature* ch;
};

// stream a chunk of a string
struct StreamChunk {
	explicit StreamChunk(const char* s_text, size_t s_len) : text(s_text), len(s_len) {}
	explicit StreamChunk(const char* s_text, const char* s_last) : text(s_text), len(s_last-s_text+1) {}

	friend const StreamControl& operator <<(const StreamControl& stream, const StreamChunk& chunk)
	{
		return stream.stream_put(chunk.text, chunk.len);
	}

	const char* text;
	size_t len;
};

// C++ style stream operators
inline
const StreamControl& operator <<(const StreamControl& stream, long i)
{
	char buf[40];
	snprintf(buf,sizeof(buf),"%ld",i);
	return stream.stream_put(buf);
}
inline
const StreamControl& operator <<(const StreamControl& stream, unsigned long ui)
{
	char buf[40];
	snprintf(buf,sizeof(buf),"%lu",ui);
	return stream.stream_put(buf);
}
inline
const StreamControl& operator <<(const StreamControl& stream, long long lli)
{
	char buf[40];
	snprintf(buf,sizeof(buf),"%lld",lli);
	return stream.stream_put(buf);
}
inline
const StreamControl& operator <<(const StreamControl& stream, int i)
{
	return stream <<(long)i;
}
inline
const StreamControl& operator <<(const StreamControl& stream, unsigned int ui)
{
	return stream <<(unsigned long)ui;
}
inline
const StreamControl& operator <<(const StreamControl& stream, const void* ptr)
{
	char buf[40];
	snprintf(buf,sizeof(buf),"%p",ptr);
	return stream.stream_put(buf);
}
inline
const StreamControl& operator <<(const StreamControl& stream, const char *cstr)
{
	return stream.stream_put(cstr);
}
inline
const StreamControl& operator <<(const StreamControl& stream, const std::string& string)
{
	return stream.stream_put(string.c_str());
}
inline
const StreamControl& operator <<(const StreamControl& stream, const char ch)
{
	const char buf[2] = {ch, '\0'};
	return stream.stream_put(buf, 1);
}

#endif // SOURCEMUD_STREAMS
