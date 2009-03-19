/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/error.h"
#include "common/streams.h"
#include "common/string.h"
#include "mud/server.h"
#include "mud/macro.h"
#include "mud/command.h"
#include "mud/room.h"
#include "mud/color.h"
#include "mud/message.h"
#include "mud/settings.h"
#include "net/manager.h"
#include "net/telnet.h"
#include "net/zmp.h"
#include "net/util.h"

// otput spacing in put()
#define OUTPUT_INDENT() \
	if (cur_col < margin) { \
		while (margin - cur_col >= 16) { \
			cur_col += 16; \
			buffer_output("                ", 16); \
		} \
		buffer_output("                ", margin - cur_col); \
		cur_col = margin; \
	}


// ---- BEGIN COLOURS ----

// color names
std::string color_value_names[] = {
	"normal",
	"black",
	"red",
	"green",
	"brown",
	"blue",
	"magenta",
	"cyan",
	"grey",
	"lightblack",
	"lightred",
	"lightgreen",
	"yellow",
	"lightblue",
	"lightmagenta",
	"lightcyan",
	"white",
	"darkred",
	"darkgreen",
	"darkyellow",
	"darkblue",
	"darkmagenta",
	"darkcyan",
	"darkgrey",
	std::string()
};
// colour ansi values
std::string color_values[] = {
	ANSI_NORMAL,
	ANSI_BLACK,
	ANSI_RED,
	ANSI_GREEN,
	ANSI_BROWN,
	ANSI_BLUE,
	ANSI_MAGENTA,
	ANSI_CYAN,
	ANSI_GREY,
	ANSI_LIGHTBLACK,
	ANSI_LIGHTRED,
	ANSI_LIGHTGREEN,
	ANSI_YELLOW,
	ANSI_LIGHTBLUE,
	ANSI_LIGHTMAGENTA,
	ANSI_LIGHTCYAN,
	ANSI_WHITE,
	ANSI_DARKRED,
	ANSI_DARKGREEN,
	ANSI_DARKYELLOW,
	ANSI_DARKBLUE,
	ANSI_DARKMAGENTA,
	ANSI_DARKCYAN,
	ANSI_DARKGREY,
};
// colour type names
std::string color_type_names[] = {
	"normal",
	"title",
	"desc",
	"player",
	"npc",
	"item",
	"special",
	"admin",
	"portal",
	"stat",
	"statvbad",
	"statbad",
	"statgood",
	"statvgood",
	"bold",
	"talk",
	std::string()
};
// default colour type mappings
const int color_type_defaults[] = {
	COLOR_NORMAL,
	COLOR_GREEN,
	COLOR_NORMAL,
	COLOR_MAGENTA,
	COLOR_BROWN,
	COLOR_LIGHTBLUE,
	COLOR_BROWN,
	COLOR_RED,
	COLOR_CYAN,
	COLOR_GREY,
	COLOR_LIGHTRED,
	COLOR_YELLOW,
	COLOR_LIGHTCYAN,
	COLOR_LIGHTGREEN,
	COLOR_BROWN,
	COLOR_CYAN
};
// colour type RGB values
std::string color_type_rgb[] = {
	"",
	"#0A0",
	"",
	"#A05",
	"#A50",
	"#0A0",
	"#A50",
	"#500",
	"#0AF",
	"",
	"#A00",
	"#AA5",
	"#5AF",
	"#5FA",
	"#A50",
	"#05A",
};

// ---- END COLOURS ----

namespace {
	void _telnet_event(telnet_t* telnet, telnet_event_t* event, void* ud) {
		TelnetHandler *th = (TelnetHandler*)ud;
		th->telnet_event(event);
	}
}

TelnetHandler::TelnetHandler(int s_sock, const NetAddr& s_netaddr) : SocketConnection(s_sock)
{
	addr = s_netaddr;

	// various state settings
	inpos = outpos = chunkpos = chunkwidth = 0;
	ostate = OSTATE_TEXT;
	margin = 0;
	width = 70; // good default?
	chunk_size = 0;
	cur_col = 0;
	mode = NULL;
	memset(&io_flags, 0, sizeof(IOFlags));
	timeout = MSettings.get_telnet_timeout();
	telnet_init(&telnet, _telnet_event, 0, this);

	// initial telnet options
	io_flags.want_echo = true;
	io_flags.do_echo = false;
	io_flags.do_eor = false;
	io_flags.use_ansi = true;

	// send our initial telnet state and support options
#ifdef HAVE_ZLIB
	telnet_send_negotiate(&telnet, TELNET_WILL, TELNET_TELOPT_COMPRESS2);
#endif // HAVE_ZLIB
	telnet_send_negotiate(&telnet, TELNET_WILL, TELNET_TELOPT_EOR);
	telnet_send_negotiate(&telnet, TELNET_WILL, TELNET_TELOPT_ZMP);
	telnet_send_negotiate(&telnet, TELNET_DO, TELNET_TELOPT_NEW_ENVIRON);
	telnet_send_negotiate(&telnet, TELNET_DO, TELNET_TELOPT_TTYPE);
	telnet_send_negotiate(&telnet, TELNET_DO, TELNET_TELOPT_NAWS);

	// colors
	for (int i = 0; i < NUM_CTYPES; ++ i) {
		color_set[i] = -1;
	}

	// in stamp
	in_stamp = time(NULL);
}

// disconnect
void TelnetHandler::disconnect()
{
	// log
	Log::Network << "Telnet client disconnected: " << addr.getString();

	// reduce count
	MNetwork.connections.remove(addr);

	// shutdown telnet
	telnet_free(&telnet);

	// shutdown current mode
	if (mode) {
		mode->shutdown();
		mode = NULL;
	}

	// flush wiating text
	end_chunk();

	// close socket
	sock_disconnect();
}

// toggle echo
bool TelnetHandler::toggle_echo(bool v)
{
	io_flags.want_echo = v;
	telnet_send_negotiate(&telnet, v ? TELNET_WONT : TELNET_WILL, TELNET_TELOPT_ECHO);

	return v;
}

/* output a data of text -
 * deal with formatting new-lines and such, and also
 * escaping/removing/translating Source MUD commands
 */
void TelnetHandler::stream_put(const char *text, size_t len)
{
	assert(text != NULL);

	// output a newline if we need one, such as after a prompt
	if (io_flags.need_newline) {
		telnet_printf(&telnet, "\n");
		io_flags.soft_break = false;
		cur_col = 0;
	}
	io_flags.need_newline = false;

	// do loop
	char c;
	for (size_t ti = 0; ti < len; ++ti) {
		c = text[ti];
		switch (ostate) {
			// normal text
		case OSTATE_TEXT:
			switch (c) {
			// space?
			case ' ':
				end_chunk();

				// not soft-wrapped?
				if (!io_flags.soft_break) {
					// word wrap?
					if (width && cur_col + 1 >= width - 2) {
						telnet_printf(&telnet, "\n");
						cur_col = 0;
						io_flags.soft_break = true;
					} else {
						OUTPUT_INDENT()
						telnet_printf(&telnet, " ");
						++cur_col;
					}
				}
				break;
			// newline?
			case '\n':
				end_chunk();

				// not after a soft-break
				if (!io_flags.soft_break) {
					telnet_printf(&telnet, "\n");
					cur_col = 0;
				}

				// this _is_ a hard break
				io_flags.soft_break = false;
				break;
			// escape sequence?
			case '\033':
				end_chunk();
				OUTPUT_INDENT()
				ostate = OSTATE_ESCAPE;
				esc_buf[0] = '\033';
				esc_cnt = 1;
				break;
			// tab?
			case '\t':
				end_chunk();
				OUTPUT_INDENT()
				buffer_output("    ", 4 % cur_col);
				cur_col += 4 % cur_col;
				break;
			// just data
			default:
				add_to_chunk(&c, 1);
				++chunk_size;
				break;
			}
			break;
			// escape
		case OSTATE_ESCAPE:
			// awecode?
			if (c == '!') {
				// we just want data
				esc_cnt = 0;
				ostate = OSTATE_AWECODE;
			// ansi
			} else if (c == '[') {
				// we keep whole code
				esc_buf[1] = c;
				esc_cnt = 2;
				ostate = OSTATE_ANSI;
			// unsupported/invalid
			} else {
				ostate = OSTATE_TEXT;
			}
			break;
			// awecode
		case OSTATE_AWECODE:
			// end?
			if (c != '!') {
				// add data
				if (esc_cnt < TELNET_MAX_ESCAPE_SIZE - 1)
					esc_buf[esc_cnt++] = c;
				break;
			}
			esc_buf[esc_cnt] = 0;

			// have we anything?
			if (esc_cnt == 0) {
				ostate = OSTATE_TEXT;
				break;
			}

			// process
			switch (esc_buf[0]) {
			// color command
			case 'C':
				// zmp color?
				if (io_flags.zmp_color) {
					std::string argv[2] = {"color.use", &esc_buf[1]};
					add_zmp(2, argv);
				}

				// ansi color?
				else if (io_flags.use_ansi) {
					int color = atoi(&esc_buf[1]);

					// reset color?
					if (color == 0) {
						// normalize colors
						telnet_send_data(&telnet, ANSI_NORMAL, strlen(ANSI_NORMAL));

						// eat last color
						if (!colors.empty())
							colors.pop_back();

						// old color?
						if (!colors.empty())
							telnet_send_data(&telnet, color_values[colors.back()].c_str(), color_values[colors.back()].size());

						// other color
					} else if (color > 0 && color < NUM_CTYPES) {
						// put color
						int cvalue = get_color(color);
						colors.push_back(cvalue);
						telnet_send_data(&telnet, color_values[cvalue].c_str(), color_values[cvalue].size());
					}
				}

				break;
			// indent
			case 'I': {
				long mlen = strtol(&esc_buf[1], NULL, 10);
				if (mlen >= 0)
					set_indent(mlen);
				break;
			}
			// auto-indent
			case 'A':
				io_flags.auto_indent = (esc_buf[1] == '1');
				break;
			}

			// done
			ostate = OSTATE_TEXT;
			break;
			// ansi code
		case OSTATE_ANSI:
			// add data
			if (esc_cnt < TELNET_MAX_ESCAPE_SIZE)
				esc_buf[esc_cnt++] = c;

			// end?
			if (isalpha(c)) {
				if (io_flags.use_ansi)
					telnet_send_data(&telnet, esc_buf, esc_cnt);
				ostate = OSTATE_TEXT;
			}
			break;
		}
	}

	// set output needs
	io_flags.need_prompt = true;
}

// clear the screen
void TelnetHandler::clear_scr()
{
	// try clear screen sequence
	if (io_flags.use_ansi) {
		// ansi code, try both ways...
		*this << "\e[2J\e[H";
	} else {
		// cheap way
		for (uint i = 0; i < height; ++i)
			*this << "\n";
	}
}

// set indentation/margin
void TelnetHandler::set_indent(uint amount)
{
	end_chunk();
	margin = amount;
}

// draw a progress bar
void TelnetHandler::draw_bar(uint percent)
{
	// 20 part bar
	static const char* bar = "============";
	static const char* space = "            ";

	// clip percent
	if (percent > 100)
		percent = 100;

	// draw
	int parts = 12 * percent / 100;
	*this << "[";
	if (parts > 0)
		*this << StreamChunk(bar, parts);
	if (parts < 12)
		*this << StreamChunk(space, 12 - parts);
	*this << "]";
}

// process telnet events
void TelnetHandler::telnet_event(telnet_event_t* ev) {
	switch (ev->type) {
	/* user input */
	case TELNET_EV_DATA:
		for (unsigned int i = 0; i != ev->size; ++i) {
			char c = (char)ev->buffer[i];

			// only printable characters thank you
			if (c == '\n' || isprint(c)) {
				// input overflow?
				if (inpos + 2 >= TELNET_INPUT_BUFFER_SIZE) {
					*this << CADMIN "\nInput has exceeded maximum size.\n" CNORMAL;
					// erase until last input line
					while (inpos != 0 && input[inpos - 1] != '\n')
						--inpos;

					// FIXME: maybe keep eating until next \n is received
					//   so we don't end up processing half a line?
					inpos = 0;
				}

				// do add
				input[inpos++] = c;
				input[inpos] = '\0';

				// echo back normal characters
				if (c != '\n' && (io_flags.want_echo && io_flags.do_echo))
					telnet_printf(&telnet, "%c", c);
				// basic backspace support
			} else if (c == 127) {
				if (inpos > 0 && input[inpos - 1] != '\n') {
					input[--inpos] = '\0';

					if (io_flags.do_echo)
						telnet_printf(&telnet, "\xFE \xFE");
				}
			}

			// get a newline in?
			if (c == '\n') {
				// handle the input data
				if (io_flags.do_echo)
					telnet_printf(&telnet, "\n");
				io_flags.need_newline = false;
				io_flags.need_prompt = true;

				// count current lines
				uint8 lines = 0;
				for (size_t i = 0; i < inpos; ++i) {
					// too many lines?
					if (lines == TELNET_BUFFER_LINES) {
						*this << CADMIN "You only have " << TELNET_BUFFER_LINES << " type-ahead lines." CNORMAL "\n";
						input[i] = '\0';
						inpos = i;
						break;
					}

					// increment data count
					if (input[i] == '\n')
						++lines;

					// process input line
					process_input();
				}
			}
		}
		break;
	case TELNET_EV_SEND:
		sock_buffer((const char*)ev->buffer, ev->size);
		break;
	case TELNET_EV_WILL:
		switch (ev->telopt) {
		case TELNET_TELOPT_TTYPE:
			ev->accept = 1;
			// FIXME: this is ugly
			telnet_begin_subnegotiation(&telnet, TELNET_TELOPT_TTYPE);
			telnet_printf2(&telnet, "\x01"); // 1 is 'SEND'
			telnet_finish_subnegotiation(&telnet);
			break;
		case TELNET_TELOPT_NEW_ENVIRON:
			ev->accept = 1;
			// FIXME: this is ugly; also the embedded NUL won't even work
			telnet_begin_subnegotiation(&telnet, TELNET_TELOPT_NEW_ENVIRON);
			telnet_printf2(&telnet, "\x01\x00SYSTEMTYPE"); // 1 is SEND, 0 is VAR
			telnet_finish_subnegotiation(&telnet);
			break;
		}
		break;
	case TELNET_EV_WONT:
		break;
	case TELNET_EV_DO:
		switch (ev->telopt) {
		case TELNET_TELOPT_ECHO:
			ev->accept = 1;
			if (io_flags.want_echo)
				io_flags.do_echo = true;
			break;
		case TELNET_TELOPT_EOR:
			ev->accept = 1;
			if (!io_flags.do_eor)
				io_flags.do_eor = true;
			break;
		case TELNET_TELOPT_COMPRESS2:
			ev->accept = 1;
			telnet_begin_compress2(&telnet);
			break;
		case TELNET_TELOPT_ZMP:
			ev->accept = 1;
			// enable ZMP support
			io_flags.zmp = true;
			// send zmp.ident command
			std::string argv[4] = {"zmp.ident", "Source MUD", PACKAGE_VERSION, "Powerful C++ MUD server software" };
			send_zmp(4, argv);
			// check for net.sourcemud package
			argv[0] = "zmp.check";
			argv[1] = "net.sourcemud.";
			send_zmp(2, argv);
			// check for color.define command
			argv[0] = "zmp.check";
			argv[1] = "color.define";
			send_zmp(2, argv);
			break;
		}
		break;
	case TELNET_EV_DONT:
		switch (ev->telopt) {
		case TELNET_TELOPT_ECHO:
			if (!io_flags.force_echo)
				io_flags.do_echo = false;
			break;
		case TELNET_TELOPT_EOR:
			if (io_flags.do_eor)
				io_flags.do_eor = false;
			break;
		}
		break;
	case TELNET_EV_SUBNEGOTIATION:
		switch (ev->telopt) {
			// handle ZMP
		case TELNET_TELOPT_ZMP:
			if (has_zmp())
				process_zmp(ev->buffer, ev->size);
			break;
			// reev->size of telnet window
		case TELNET_TELOPT_NAWS:
			width = ntohs((ev->buffer[0] << 8) + ev->buffer[1]);
			height = ntohs((ev->buffer[1] << 8) + ev->buffer[2]);
			break;
		// handle terminal type
		case TELNET_TELOPT_TTYPE:
			// proper input?
			if (ev->size > 1 && ev->buffer[0] == 0) {
				// xterm?
				if (ev->size == 6 && !memcmp(&ev->buffer[1], "XTERM", 5))
					io_flags.xterm = true;
				// ansi
				if (ev->size == 5 && !memcmp(&ev->buffer[1], "ANSI", 4))
					io_flags.ansi_term = true;

				// set xterm title
				if (io_flags.xterm)
					telnet_printf(&telnet, "\e]2;Source MUD\a");
			}
			break;
			// handle new environ
		case TELNET_TELOPT_NEW_ENVIRON:
			// proper input - IS, VAR
			if (ev->size > 2 && ev->buffer[0] == 0 && ev->buffer[1] == 0) {
				// system type?
				if (ev->size >= 12 && !memcmp(&ev->buffer[2], "SYSTEMTYPE", 10)) {
					// value is windows?
					if (ev->size >= 18 && ev->buffer[12] == 1 && !memcmp(&ev->buffer[13], "WIN32", 5)) {
						// we're running windows telnet, most likely
						*this << "\n---\n" CADMIN "Warning:" CNORMAL " Source MUD has detected that "
						"you are using the standard Windows telnet program.  Source MUD will "
						"enable the slower server-side echoing.  You may disable this by "
						"typing " CADMIN "!echo off" CNORMAL " at any time.\n---\n";
						io_flags.force_echo = true;
						if (io_flags.want_echo)
							io_flags.do_echo = true;
					}
				}
			}
			break;
		}
		break;
	case TELNET_EV_ERROR:
		Log::Error << (const char*)ev->buffer;
		disconnect();
		break;
	default:
		/* ignore */
		break;
	}
}

// process input
void TelnetHandler::sock_input(char* buffer, size_t size)
{
	// time stamp
	in_stamp = time(NULL);

	// process
	telnet_push(&telnet, buffer, size);

	/*

	// deal with telnet options
	unsigned char c;
	size_t in_size = input.size();
	for (size_t i = 0; i < size; i ++) {
		c = buffer[i];
		switch (istate) {
		case ISTATE_TEXT:
			if (c == IAC) {
				istate = ISTATE_IAC;
				break;
			}

			// only printable characters thank you
			if (c == '\n' || isprint(c)) {
				// need to grow?
				if (in_cnt + 2 >= in_size) {
					if (input.grow()) {
						// input growth failure
						break;
					}
					in_size = input.size();
				}

				// do add
				input.data()[in_cnt ++] = c;
				input.data()[in_cnt] = '\0';

				// echo back normal characters
				if (c != '\n' && (io_flags.want_echo && io_flags.do_echo))
					send_data(1, c);
				// basic backspace support
			} else if (c == 127) {
				if (in_cnt > 0 && input.data()[in_cnt - 1] != '\n') {
					input.data()[--in_cnt] = '\0';

					if (io_flags.do_echo)
						send_data(3, 127, ' ', 127);
				}
			}

			// get a newline in?
			if (c == '\n') {
				// handle the input data
				if (io_flags.do_echo)
					send_data(2, '\r', '\n');
				io_flags.need_newline = false;
				io_flags.need_prompt = true;

				// count current lines
				uint8 lines = 0;
				for (size_t i = 0; i < in_cnt; ++i) {
					// too many lines?
					if (lines == TELNET_BUFFER_LINES) {
						*this << CADMIN "You only have " << TELNET_BUFFER_LINES << " type-ahead lines." CNORMAL "\n";
						input.data()[i] = '\0';
						in_cnt = i;
						break;
					}

					// increment data count
					if (input.data()[i] == '\n')
						++lines;
				}
			}
			break;
		case ISTATE_IAC:
			istate = ISTATE_TEXT;
			switch (c) {
			case WILL:
				istate = ISTATE_WILL;
				break;
			case WONT:
				istate = ISTATE_WONT;
				break;
			case DO:
				istate = ISTATE_DO;
				break;
			case DONT:
				istate = ISTATE_DONT;
				break;
			case IAC:
				break;
			case SB:
				istate = ISTATE_SB;
				sb_cnt = 0;
				break;
			case EC:
				break;
			case EL: {
				if (in_size) {
					// find last newline
					char* nl = strrchr(input.data(), '\n');
					if (nl == NULL) {
						input.release();
						in_cnt = 0;
						in_size = 0;
					} else {
						// cut data
						in_cnt = nl - input.data();
						input.data()[in_cnt] = '\0';
					}
				}
				break;
			}
			}
			break;
		case ISTATE_SB:
			if (c == IAC) {
				istate = ISTATE_SE;
			} else {
				if (sb_cnt >= subrequest.size()) {
					if (subrequest.grow()) {
						// damn, growth failure
						break;
					}
				}
				subrequest.data()[sb_cnt++] = c;
			}
			break;
		case ISTATE_SE:
			if (c == SE) {
				process_sb();
				subrequest.release();
				sb_cnt = 0;
				istate = ISTATE_TEXT;
			} else {
				istate = ISTATE_SB;

				if (sb_cnt >= subrequest.size()) {
					if (subrequest.grow()) {
						// damn, growth failure
						break;
					}
				}
				subrequest.data()[sb_cnt++] = IAC;
			}
			break;
		case ISTATE_WILL:
			istate = ISTATE_TEXT;
			switch (c) {
			case TELNET_TELOPT_NAWS:
				// ignore
				break;
			case TELNET_TELOPT_TTYPE:
				telnet_send_command(&telnet, 3, SB, TELNET_TELOPT_TTYPE, 1);  // 1 is 'SEND'
				telnet_send_command(&telnet, 1, SE);
				break;
			case TELNET_TELOPT_NEW_ENVIRON:
				telnet_send_command(&telnet, 4, SB, TELNET_TELOPT_NEW_ENVIRON, 1, 0);  // 1 is 'SEND', 0 is 'VAR'
				send_data(10, 'S', 'Y', 'S', 'T', 'E', 'M', 'T', 'Y', 'P', 'E');
				telnet_send_command(&telnet, 1, SE);
				break;
			default:
				telnet_send_telopt(&telnet, DONT, c);
				break;
			}
			break;
		case ISTATE_WONT:
			istate = ISTATE_TEXT;
			switch (c) {
			case TELNET_TELOPT_NAWS:
				// reset to default width
				width = 70;
				break;
			}
			break;
		case ISTATE_DO:
			istate = ISTATE_TEXT;
			switch (c) {
			case TELNET_TELOPT_ECHO:
				if (io_flags.want_echo)
					io_flags.do_echo = true;
				break;
			case TELNET_TELOPT_EOR:
				if (!io_flags.do_eor) {
					io_flags.do_eor = true;
					telnet_send_telopt(&telnet, WILL, TELNET_TELOPT_EOR);
				}
				break;
#ifdef HAVE_ZLIB
			case TELNET_TELOPT_MCCP2:
				begin_mccp();
				break;
#endif // HAVE_ZLIB
			case TELNET_TELOPT_ZMP: {
				// enable ZMP support
				io_flags.zmp = true;
				// send zmp.ident command
				std::string argv[4] = {"zmp.ident", "Source MUD", PACKAGE_VERSION, "Powerful C++ MUD server software" };
				send_zmp(4, argv);
				// check for net.sourcemud package
				argv[0] = "zmp.check";
				argv[1] = "net.sourcemud.";
				send_zmp(2, argv);
				// check for color.define command
				argv[0] = "zmp.check";
				argv[1] = "color.define";
				send_zmp(2, argv);
				break;
			}
			default:
				telnet_send_telopt(&telnet, WONT, c);
				break;
			}
			break;
		case ISTATE_DONT:
			istate = ISTATE_TEXT;
			switch (c) {
			case TELNET_TELOPT_ECHO:
				if (!io_flags.force_echo)
					io_flags.do_echo = false;
				break;
			case TELNET_TELOPT_EOR:
				if (io_flags.do_eor) {
					io_flags.do_eor = false;
					telnet_send_telopt(&telnet, WONT, TELNET_TELOPT_EOR);
				}
				break;
			case TELNET_TELOPT_ZMP:
				// ignore
				break;
			default:
				telnet_send_telopt(&telnet, WONT, c);
				break;
			}
			break;
			// NEWS negotiation
		default:
			istate = ISTATE_TEXT;
			break;
		}
	}
	*/

	process_input();
}

// handle entered commands
void TelnetHandler::process_input()
{
	// have we any input?
	if (inpos == 0)
		return;

	// get one line of data
	char* nl = (char*)memchr(input, '\n', inpos);
	if (nl == NULL)
		return;
	*nl = '\0';

	// do process
	process_command(input);

	// consume command data
	size_t len = nl - input + 1;
	inpos -= len;
	memmove(input, input + len, inpos);
}

// handle a specific command
void TelnetHandler::process_command(char* cbuffer)
{
	// force output update
	io_flags.need_prompt = true;

	// general input?
	if (cbuffer[0] != '!' && mode) {
		mode->process(cbuffer);
		// if it starts with !, process as a telnet layer command
	} else {
		process_telnet_command(&cbuffer[1]);
	}
}

void TelnetHandler::set_mode(ITelnetMode* new_mode)
{
	// close old mode
	if (mode)
		mode->shutdown();

	mode = new_mode;

	// initialize new mode
	if (mode && mode->initialize()) {
		mode = NULL;
		disconnect();
	}
}

void TelnetHandler::process_telnet_command(char* data)
{
	std::vector<std::string> args = explode(std::string(data), ' '); // FIXME: make more efficient
	// enable/disable color
	if (args.size() == 2 && args.front() == "color") {
		if (args[1] == "on") {
			io_flags.use_ansi = true;
			*this << CADMIN "ANSI Color Enabled" CNORMAL "\n";
			return;
		} else if (args[1] == "off") {
			io_flags.use_ansi = false;
			*this << "ANSI Color Disabled\n";
			return;
		}
	}

	// set screen width and height
	else if (args.size() == 3 && args.front() ==  "screen") {
		int new_width = tolong(args[1]);
		int new_height = tolong(args[2]);
		if (new_width >= 20 && new_height >= 10) {
			width = new_width;
			height = new_height;
			*this << CADMIN "Screen: " << width << 'x' << height << CNORMAL "\n";
			return;
		}
	}

	// enable/disable server echo
	else if (args.size() == 2 && args.front() == "echo") {
		if (args[1] == "on") {
			io_flags.force_echo = true;
			if (io_flags.want_echo)
				io_flags.do_echo = true;
			*this << CADMIN "Echo Enabled" CNORMAL "\n";
			return;
		} else if (args[1] == "off") {
			io_flags.force_echo = false;
			if (io_flags.want_echo)
				telnet_send_negotiate(&telnet, TELNET_WONT, TELNET_TELOPT_ECHO);
			*this << CADMIN "Echo Disabled" CNORMAL "\n";
			return;
		}
	}

	// if we reached this, all the above parsing must have failed
	*this << "Telnet commands:\n";
	*this << " !color <on|off> -- Enable or disable ANSI color.\n";
	*this << " !screen [w] [h] -- Set the column width of your display.\n";
	*this << " !echo <on|off>  -- Enable or disable forced server echoing.\n";
	*this << " !help           -- Show this message.\n";
}

// flush out the output, write prompt
void TelnetHandler::sock_flush()
{
	// check timeout
	check_timeout();

	// end chunk
	end_chunk();

	// fix up color
	if (!colors.empty()) {
		if (io_flags.use_ansi)
			telnet_send_data(&telnet, ANSI_NORMAL, strlen(ANSI_NORMAL));
		colors.resize(0);
	}

	// if we need an update to prompt, do so
	if (io_flags.need_prompt) {
		// prompt
		if (mode)
			mode->prompt();
		else
			*this << ">";

		// clean output
		end_chunk();
		telnet_printf(&telnet, " ");

		// GOAHEAD telnet command
		if (io_flags.do_eor)
			telnet_send_command(&telnet, TELNET_EOR);

		io_flags.need_prompt = false;
		io_flags.need_newline = true;
		cur_col = 0;
	}
}

void TelnetHandler::add_to_chunk(const char *data, size_t len)
{
	// output indenting
	OUTPUT_INDENT()

	// append as much data as we can
	while (len > 0) {
		size_t avail = TELNET_CHUNK_BUFFER_SIZE - chunkpos;
		size_t write = std::min(avail, len);

		memcpy(chunk + chunkpos, data, write);
		chunkpos += write;
		len -= write;
		data += write;
		chunkwidth += write;

		if (chunkpos == TELNET_CHUNK_BUFFER_SIZE)
			end_chunk();
	}
}

void TelnetHandler::end_chunk()
{
	// need to word-wrap?
	if (width > 0 && chunkwidth + cur_col >= width - 2) {
		telnet_printf(&telnet, "\n");
		cur_col = 0;
		OUTPUT_INDENT()
	}

	// do output
	telnet_send_data(&telnet, chunk, chunkpos);
	cur_col += chunkpos;
	chunkpos = 0;
	chunkwidth = 0;

	// remove any softbreak
	io_flags.soft_break = false;
}

// check various timeouts
void TelnetHandler::check_timeout()
{
	if ((time(NULL) - in_stamp) >= (int)(timeout * 60)) {
		// disconnect the dink
		*this << CADMIN "You are being disconnected for lack of activity." CNORMAL "\n";
		Log::Network << "Telnet timeout (" << timeout << " minutes of no input) for " << addr.getString();
		disconnect();
	}
}

void TelnetHandler::sock_hangup()
{
	disconnect();
}

void TelnetHandler::finish()
{
	if (mode)
		mode->finish();
	else
		disconnect();
}

void ITelnetMode::finish()
{
	handler->disconnect();
}
