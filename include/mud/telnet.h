/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef TELNET_H
#define TELNET_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif // HAVE_LIBZ

#include "common/gcvector.h"
#include "common/types.h"
#include "common/gcbase.h"
#include "common/streams.h"
#include "mud/network.h"
#include "mud/color.h"
#include "scriptix/native.h"

// default window size
#define TELNET_DEFAULT_WIDTH 80
#define TELNET_DEFAULT_HEIGHT 24

// default timeout in minutes
#define TELNET_DEFAULT_TIMEOUT 15

// max pre-input lines
#define TELNET_BUFFER_LINES 3

// maximum escape length
#define TELNET_MAX_ESCAPE_SIZE 32

class TextBufferList
{
	private:
	GCType::vector<char*> list;
	const size_t size;
	size_t allocs, pallocs, out;

	public:
	TextBufferList (size_t s_size) : list (), size(s_size), allocs(0), pallocs(0), out(0) {}
	~TextBufferList (void) {
		for (GCType::vector<char*>::iterator i = list.begin(); i != list.end(); ++i)
			if (*i)
				delete[] *i;
	}

	size_t get_size (void) const { return size; }

	char* alloc (void);
	void release (char* buf) {
		-- out;
		list.push_back(buf);
	}
};
class TextBuffer 
{
	public:
	typedef enum {
		EMPTY = -1,
		SIZE_2048 = 0,
		SIZE_4096,
		SIZE_8192,
		SIZE_16384,
		COUNT
	} SizeType;
	
	private:
	SizeType bsize;
	char* bdata;

	// the buffers
	static TextBufferList lists[COUNT];

	public:
	TextBuffer (void) : bsize(EMPTY), bdata(NULL) {}
	~TextBuffer (void) { release(); }

	size_t size(void) const { return bsize != EMPTY ? lists[bsize].get_size() : 0; }
	char *data(void) const { return bdata; }
	void release (void);
	int alloc(SizeType size);
	int grow (void);

	TextBuffer& copy (TextBuffer& buffer) {
		if (bdata != NULL)
			release();
		bsize = buffer.bsize;
		bdata = buffer.bdata;
		buffer.bsize = EMPTY;
		buffer.bdata = NULL;
		return *this;
	}
};

class ITelnetMode : public GC
{
	public:
	ITelnetMode (class TelnetHandler* s_handler) : handler(s_handler) {}
	virtual ~ITelnetMode (void) {}

	// basics
	virtual int initialize (void) = 0;
	virtual void prompt (void) = 0;
	virtual void process (char* line) = 0;
	virtual void shutdown (void) = 0;
	virtual void check (void) = 0;

	// the handler this mode is connected to
	inline class TelnetHandler* get_handler (void) const { return handler; }

	private:
	class TelnetHandler* handler;
};

class TelnetHandler : public Scriptix::Native, public SocketUser, public IStreamSink
{
	public:
	TelnetHandler (int s_sock, const SockStorage& s_netaddr);

	// color info
	inline uint get_color (uint i) const { return color_set[i] < 0 ? color_type_defaults[i] : color_set[i]; }
	inline void set_color (uint i, uint v) { color_set[i] = v; }
	inline void clear_color (uint i) { color_set[i] = -1; }
	inline bool use_color (void) const { return io_flags.use_ansi; }

	// processing IO
	void process (void);
	inline int get_width (void) const { return width; }
	bool toggle_echo (bool value);
	void process_command (char* cmd); // just as if typed in by user
	void disconnect (void);

	// output
	virtual void stream_put (const char*, size_t len);
	void clear_scr (void); // clear da screen
	void set_indent (uint amount);
	inline uint get_indent (void) const { return margin; }
	void draw_bar (uint percent); // draws a 14 character width progress bar
	inline void force_update () { io_flags.need_prompt = true; }

	// change timeout
	inline void set_timeout (uint s_timeout) { timeout = s_timeout; }

	// ZMP
	inline bool has_zmp (void) const { return io_flags.zmp; }
	inline bool has_zmp_net_awemud (void) const { return io_flags.zmp_net_awemud; } // supports the net.awemud. package?
	inline bool has_zmp_color (void) const { return io_flags.zmp_color; } // supports the color.define command?
	void send_zmp (size_t argc, const char** argv);
	inline void send_zmp (size_t argc, char** argv) { send_zmp(argc, (const char**)argv); }
	void zmp_support (const char* pkg, bool value);

	// mode
	void set_mode (ITelnetMode* new_mode);
		// ask mode to see if needs to do anything special - FIXME: this
		// is a hack necessitated by Player needing to be able to tell
		// TelnetModePlay to exit and reinstate TelnetModeMainMenu.  Our
		// encapsulation here sucks.
	void check_mode (void);

	// low-level IO
	virtual void in_handle (char* buffer, size_t size);
	virtual char get_poll_flags (void);
	virtual void out_ready (void);
	virtual void hangup (void);
	virtual void prepare (void);

	protected:
	// destructor
	~TelnetHandler (void) {}

	protected:
	TextBuffer input; // player input buffer
	TextBuffer output; // output
	TextBuffer outchunk; // chunk of output
	TextBuffer subrequest; // telnet-subrequest input
	uint in_cnt, out_cnt, sb_cnt, outchunk_cnt; // counts for buffers
	char esc_buf[TELNET_MAX_ESCAPE_SIZE]; // output escape sequences
	uint esc_cnt; // count of escape characters
	uint width, height; // terminal size
	uint cur_col; // current output column
	uint margin; // forced indent
	uint chunk_size; // size of printable chunk characters
	uint timeout; // timeout length for this connection
	time_t in_stamp; // last input time
	int color_set[NUM_CTYPES]; // color codes
	GCType::vector<int> colors; // current color stack
#ifdef HAVE_LIBZ
	z_stream* zstate; // compression
#endif
	struct IOFlags {
		int use_ansi:1, need_prompt:1, need_newline:1, do_echo:1,
			do_eor:1, want_echo:1, xterm:1, force_echo:1, zmp:1,
			ready:1, soft_break:1, ansi_term:1, zmp_net_awemud:1,
			zmp_color:1, auto_indent:1;
	} io_flags;

	// input states - telnet
	enum {
		ISTATE_TEXT,
		ISTATE_IAC,
		ISTATE_WILL,
		ISTATE_WONT,
		ISTATE_DO,
		ISTATE_DONT,
		ISTATE_SB,
		ISTATE_SE,
	} istate;

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
	SockStorage addr;

#ifdef HAVE_LIBZ
	// compression
	bool begin_mccp (void);
	void end_mccp (void);
#endif // HAVE_LIBZ

	// processing
	void process_input (void);
	void process_sb (void);
	void process_zmp (size_t size, char* chunk);

	// command handling
	void process_telnet_command (char* cmd);

	// data output
	void add_output (const char* data, size_t len);
	void add_to_out_buffer (const char* data, size_t len);
	void add_to_chunk (const char* data, size_t len);
	void end_chunk (void);
	void send_iac (uint, ...); // build iac
	void send_data (uint, ...); // don't escape
	void add_zmp (size_t argc, const char** argv);

	// timeout handling
	int check_time (void); // check to see if we should disconnect
};

// LOGIN
class TelnetModeLogin : public ITelnetMode
{
	public:
	TelnetModeLogin (TelnetHandler* s_handler) : ITelnetMode (s_handler), account(NULL), pass(false), tries(0) {}

	virtual int initialize (void);
	virtual void prompt (void);
	virtual void process (char* line);
	virtual void shutdown (void);
	virtual void check (void) {}

	private:
	class Account* account;
	bool pass;
	int tries;
};

// indent stream
class StreamIndent
{
	public:
	inline StreamIndent(uint16 s_len) : len(s_len) {}

	inline friend
	const StreamControl&
	operator << (const StreamControl& stream, const StreamIndent& indent)
	{
		return stream << "\e!" << indent.len << "!";
	}

	private:
	uint16 len;
};

#endif
