/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/string.h"
#include "common/streams.h"
#include "common/file.h"
#include "mud/player.h"
#include "mud/command.h"
#include "mud/settings.h"
#include "mud/color.h"
#include "mud/fileobj.h"
#include "mud/macro.h"
#include "mud/help.h"
#include "net/telnet.h"

_MHelp MHelp;

void command_help(Player* player, std::string argv[])
{
	StreamControl stream(player);
	MHelp.print(stream, argv[0]);
}

HelpTopic* _MHelp::get_topic(const std::string& name)
{
	for (TopicList::iterator i = topics.begin(); i != topics.end(); ++i)
		if (phrase_match((*i)->name, name))
			return *i;
	return NULL;
}

void _MHelp::print(StreamControl& stream, const std::string& name)
{
	// try a man page
	if (!name.empty() && MCommand.show_man(stream, name, true))
		return;

	// try a help topic
	HelpTopic* topic = name.empty() ? get_topic("general") : get_topic(name);
	if (topic) {
		stream << CSPECIAL "Help: " CNORMAL << topic->name << "\n\n";
		stream << StreamIndent(2) << StreamMacro(topic->about) << StreamIndent(0) << "\n";
		return;
	}

	// nope, nothin'
	stream << CSPECIAL "No help for '" << name << "' available." CNORMAL "\n";
}

int _MHelp::initialize()
{
	std::vector<std::string> files = File::dirlist(MSettings.get_help_path());
	File::filter(files, "*.help");
	for (std::vector<std::string>::iterator i = files.begin(); i != files.end(); ++i) {
		File::Reader reader;

		// open file
		std::ifstream in;
		in.open(i->c_str());
		if (!in) {
			Log::Error << "Failed to open " << *i;
			return -1;
		}

		// read file
		size_t line = 1;
		int c;
		while (!in.eof()) {
			c = in.get();

			// eof
			if (c == -1) {
				break;

				// comment
			} else if (c == '#') {
				do {
					c = in.get();
				} while (!in.eof() && c != '\n');
				++line;

				// blank line
			} else if (c == '\n') {
				++line;

				// white-space line
			} else if (isspace(c)) {
				do {
					if (!isspace(c)) {
						Log::Error << *i << ',' << line << ": Garbage on line";
						return -1;
					}
					c = in.get();
				} while (!in.eof() && c != '\n');
				++line;

				// command token
			} else if (c == '%') {
				StringBuffer buf;

				// consume rest of % command
				while (!in.eof() && isalpha(c = in.get()))
					buf << (char)c;
				if (!isspace(c)) {
					Log::Error << *i << ',' << line << ": Invalid command";
					return -1;
				}

				// command must be 'begin'
				if ("begin" != buf.str()) {
					Log::Error << *i << ',' << line << ": Unrecognized command '" << buf.c_str() << "'";
					return -1;
				}

				// consume whitespace
				while (!in.eof() && c != '\n' && isspace(c))
					c = in.get();
				if (c == '\n') {
					Log::Error << *i << ',' << line << ": Expected topic name";
					return -1;
				}

				// read topic name
				buf.clear();
				while (!in.eof() && c != '\n') {
					buf << (char)c;
					c = in.get();
				}
				if (in.eof()) {
					Log::Error << *i << ',' << line << ": Unexpected EOF";
					return -1;
				}
				++line;

				std::string name = strip(buf.str());

				// read data
				buf.clear();
				enum { PRE, WS, TEXT, NL, END } state = PRE;
				size_t pre = 0;
				size_t cur = 0;
				while (!in.eof() && state != END) {
					c = in.get();
					switch (state) {
					case PRE:
						// newline
						if (c == '\n') {
							++line;
							state = NL;
							// text
						} else if (!isspace(c)) {
							buf << (char)c;
							state = TEXT;
							// whitespace
						} else {
							++pre;
						}
						break;
					case WS:
						// newline
						if (c == '\n') {
							++line;
							buf << '\n';
							state = NL;
							// text
						} else if (!isspace(c)) {
							buf << (char)c;
							state = TEXT;
							// whitespace
						} else {
							++cur;
							if (cur > pre) {
								buf << (char)c;
								state = TEXT;
							}
						}
						break;
					case NL:
						// end
						if (c == '%') {
							state = END;
							// newline
						} else if (c == '\n') {
							buf << '\n';
							++line;
							// pre-line whitespace
						} else if (isspace(c)) {
							cur = 1;
							state = WS;
							// text
						} else {
							buf << (char)c;
							state = TEXT;
						}
						break;
					case TEXT:
						buf << (char)c;
						if (c == '\n') {
							++line;
							state = NL;
						}
						break;
					case END:
						break;
					}
				}
				if (in.eof()) {
					Log::Error << *i << ',' << line << ": Unexpected EOF";
					return -1;
				}
				do {
					c = in.get();
				} while (!in.eof() && c != '\n');
				++line;

				std::string body = buf.str();

				HelpTopic* topic = new HelpTopic();
				topic->name = name;
				topic->about = body;
				topics.push_back(topic);

				// garbage
			} else {
				Log::Error << *i << ',' << line << ": Garbage on line";
				return -1;
			}
		}
	}

	return 0;
}

void _MHelp::shutdown()
{
	// delete all topics
	for (TopicList::iterator i = topics.begin(); i != topics.end(); ++i)
		delete(*i);
	topics.clear();
}
