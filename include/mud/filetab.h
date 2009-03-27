/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_FILETAB_H
#define SOURCEMUD_MUD_FILETAB_H

namespace File
{
class TabReader
{
public:
	TabReader() : in(), filename(), line(0) {}
	~TabReader() { close(); }

	const std::string getFilename() const { return filename; }
	int open(const std::string& file);
	bool isOpen() const { return in; }
	void close() { if (in) in.close(); }

	// read in the file
	int load();

	// get the number of lines
	size_t size() const { return entries.size(); }

	// get the entry on a given line/column
	std::string get(size_t line, size_t col) const;

	// get the actual line of a particular entry
	size_t getLine(size_t line) { return entries[line].first; }

private:
	std::ifstream in;
	std::string filename;
	size_t line;

	typedef std::pair<size_t, std::vector<std::string> > Entry;
	typedef std::vector<Entry> Entries;

	Entries entries;
};
}

#endif
