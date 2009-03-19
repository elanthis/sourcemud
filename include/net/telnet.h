/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_TELNET_H
#define SOURCEMUD_MUD_TELNET_H

#include "common/types.h"
#include "common/streams.h"
#include "common/string.h"
#include "mud/color.h"
#include "net/netaddr.h"
#include "net/socket.h"
#include "libtelnet.h"

// default window size
const size_t TELNET_DEFAULT_WIDTH = 80;
const size_t TELNET_DEFAULT_HEIGHT = 24;

// default timeout in minutes
const size_t TELNET_DEFAULT_TIMEOUT = 15;

// max pre-input lines
const size_t TELNET_BUFFER_LINES = 3;

// input/output buffer sizes
const size_t TELNET_INPUT_BUFFER_SIZE = 256;
const size_t TELNET_OUTPUT_BUFFER_SIZE = 16384;
const size_t TELNET_CHUNK_BUFFER_SIZE = 32;

// maximum escape length
const size_t TELNET_MAX_ESCAPE_SIZE = 32;

class ITelnetMode
{
public:
	ITelnetMode(class TelnetHandler* s_handler) : handler(s_handler) {}
	virtual ~ITelnetMode() {}

	// basics
	virtual int initialize() = 0;
	virtual void prompt() = 0;
	virtual void process(char* line) = 0;
	virtual void shutdown() = 0;
	virtual void finish(); // disconnect session by default

	// the handler this mode is connected to
	inline class TelnetHandler* get_handler() const { return handler; }

private:
	class TelnetHandler* handler;
};

class TelnetHandler : public SocketConnection, public IStreamSink
{
public:
	TelnetHandler(int s_sock, const NetAddr& s_netaddr);

	// color info
	inline uint get_color(uint i) const { return color_set[i] < 0 ? color_type_defaults[i] : color_set[i]; }
	inline void set_color(uint i, uint v) { color_set[i] = v; }
	inline void clear_color(uint i) { color_set[i] = -1; }
	inline bool use_color() const { return io_flags.use_ansi; }

	// processing IO
	void process();
	inline int get_width() const { return width; }
	bool toggle_echo(bool value);
	void process_command(char* cmd); // just as if typed in by user
	void disconnect();
	void finish(); // tells the current mode to 'end', disconnects by default

	// output
	virtual void stream_put(const char*, size_t len);
	void clear_scr(); // clear da screen
	void set_indent(uint amount);
	inline uint get_indent() const { return margin; }
	void draw_bar(uint percent); // draws a 14 character width progress bar
	inline void force_update() { io_flags.need_prompt = true; }

	// change timeout
	inline void set_timeout(uint s_timeout) { timeout = s_timeout; }

	// ZMP
	inline bool has_zmp() const { return io_flags.zmp; }
	inline bool has_zmp_color() const { return io_flags.zmp_color; } // supports the color.define command?
	void send_zmp(size_t argc, std::string argv[]);
	void zmp_support(const std::string& pkg, bool value);

	// mode
	void set_mode(ITelnetMode* new_mode);

	// low-level IO
	virtual void sock_input(char* buffer, size_t size);
	virtual void sock_hangup();
	virtual void sock_flush();

protected:
	// destructor
	~TelnetHandler() {}

protected:
	telnet_t telnet;
	char input[TELNET_INPUT_BUFFER_SIZE];
	char output[TELNET_OUTPUT_BUFFER_SIZE];
	char chunk[TELNET_CHUNK_BUFFER_SIZE];
	uint inpos, outpos, chunkpos, chunkwidth;
	char esc_buf[TELNET_MAX_ESCAPE_SIZE]; // output escape sequences
	uint esc_cnt; // count of escape characters
	uint width, height; // terminal size
	uint cur_col; // current output column
	uint margin; // forced indent
	uint chunk_size; // size of printable chunk characters
	uint timeout; // timeout length for this connection
	time_t in_stamp; // last input time
	int color_set[NUM_CTYPES]; // color codes
	std::vector<int> colors; // current color stack
	struct IOFlags {
	int use_ansi: 1,
		need_prompt: 1,
		need_newline: 1,
		do_echo: 1,
		do_eor: 1,
		want_echo: 1,
		xterm: 1,
		force_echo: 1,
		zmp: 1,
		ready: 1,
		soft_break: 1,
		ansi_term: 1,
		zmp_color: 1,
		auto_indent: 1;
	} io_flags;

	// output states - formatting
	enum {
		OSTATE_TEXT,
		OSTATE_ESCAPE,
		OSTATE_ANSI,
		OSTATE_AWECODE
	} ostate;

	// current mode
	ITelnetMode* mode;

	// network info
	NetAddr addr;

	// processing
	void process_input();
	void process_sb(const char* data, size_t size);
	void process_zmp(size_t size, char* chunk);

	// command handling
	void process_telnet_command(char* cmd);

	// data output
	void add_to_chunk(const char* data, size_t len);
	void buffer_output(const char* data, size_t len) { telnet_send_data(&telnet, data, len); }
	void end_chunk();
	void add_zmp(size_t argc, std::string argv[]);

	// timeout handling
	virtual void check_timeout(); // check to see if we should disconnect

public:
	void telnet_event(telnet_event_t* ev);
};

// indent stream
class StreamIndent
{
public:
	inline StreamIndent(uint16 s_len) : len(s_len) {}

	inline friend
	const StreamControl&
	operator << (const StreamControl& stream, const StreamIndent& indent) {
		return stream << "\e!I" << indent.len << "!";
	}

private:
	uint16 len;
};

#endif
