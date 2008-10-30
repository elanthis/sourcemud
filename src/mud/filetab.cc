/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include <iostream>

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "common/string.h"
#include "common/strbuf.h"
#include "mud/filetab.h"
#include "common/log.h"

int
File::TabReader::open (String filename)
{
	// open
	in.open(filename.c_str());
	if (!in) {
		Log::Error << "Failed to open " << filename;
		return -1;
	}
	// finish
	TabReader::filename = filename;
	line = 1;
	return 0;
}

int
File::TabReader::load ()
{
	StringList cent;
	StringBuffer cword;
	enum {
		WS,
		WORD,
		QUOTE,
		COMMENT
	} state = WS;
	int c;

	while (!in.eof()) {
		c = in.get();
		if (!isprint(c) && c != '\n')
			continue;

		switch (state) {
			case WS:
				if (c == '\n') {
					if (!cent.empty()) {
						entries.push_back(Entry(line, cent));
						cent.clear();
					}
					++line;
				} else if (c == '"')
					state = QUOTE;
				else if (c == '#')
					state = COMMENT;
				else if (!isspace(c)) {
					cword << (char)c;
					state = WORD;
				}
				break;
			case WORD:
				if (isspace(c) || c == '#') {
					cent.push_back(cword.str());
					cword.clear();
				}
				if (c == '\n') {
					entries.push_back(Entry(line, cent));
					cent.clear();
					++line;
					state = WS;
				} else if (c == '#') {
					state = COMMENT;
				} else if (c == '"') {
					state = QUOTE;
				} else if (isspace(c)) {
					state = WS;
				} else {
					cword << (char)c;
				}
				break;
			case QUOTE:
				if (c == '\n') {
					cent.push_back(cword.str());
					cword.clear();
					entries.push_back(Entry(line, cent));
					cent.clear();
					Log::Warning << "Unterminated quoted string at " << filename << ":" << line;
					++line;
					state = WS;
				} else if (c == '"') {
					state = WORD;
				} else {
					cword << (char)c;
				}
				break;
			case COMMENT:
				if (c == '\n') {
					if (!cent.empty()) {
						entries.push_back(Entry(line, cent));
						cent.clear();
					}
					state = WS;
					++line;
				}
				break;
		}
	}

	if (state != WS) {
		Log::Warning << "Unterminated line at " << filename << ":" << line;
		if (state == WORD || state == QUOTE)
			cent.push_back(cword.str());
		if (!cent.empty())
			entries.push_back(Entry(line, cent));
	}

	return 0;
}

String
File::TabReader::get (size_t line, size_t col) const
{
	if (entries[line].second.size() <= col)
		return String();
	return entries[line].second[col];
}
