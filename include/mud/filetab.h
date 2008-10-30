/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_FILETAB_H
#define SOURCEMUD_MUD_FILETAB_H

#include <sys/stat.h>

#include <fstream>

#include "common/string.h"
#include "common/gcbase.h"
#include "common/gcvector.h"

namespace File
{
	class TabReader : public Cleanup
	{
		public:
		TabReader () : in(), filename(), line(0) {}
		~TabReader () { close(); }

		const String get_filename () const { return filename; }
		int open (String file);
		bool is_open () const { return in; }
		void close () { if (in) in.close(); }

		// read in the file
		int load ();

		// get the number of lines
		size_t size () const { return entries.size(); }

		// get the entry on a given line/column
		String get (size_t line, size_t col) const;

		// get the actual line of a particular entry
		size_t get_line (size_t line) { return entries[line].first; }

		private:
		std::ifstream in;
		String filename;
		size_t line;

		typedef std::pair<size_t, StringList> Entry;
		typedef GCType::vector<Entry> Entries;

		Entries entries;
	};
}

#endif
