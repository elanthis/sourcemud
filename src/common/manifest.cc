/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>

#include "common/manifest.h"
#include "common/log.h"

namespace {
	bool file_exists(String path)
	{
		struct stat st;
		int err = stat(path, &st);
		if (err == -1)
			return false;
		return S_ISREG(st.st_mode);
	}

	void trim (char* str)
	{
		// find end of string; # is a comment, so that's the end
		char* end = strchr(str, '#');
		if (end == NULL)
			end = str + strlen(str);

		// find the start of the actual data in the string
		char* start = str;
		while (start < end && isspace(*start))
			++start;

		// if there's no data, just "blank" the string and return
		if (start == end) {
			str[0] = 0;
			return;
		}

		// find the end of the actual data
		do {
			--end;
		} while (isspace(*end));

		// move the data to the start of the string, mark end
		memmove(str, start, end-start+1);
		str[end-start+1] = 0;
	}

	void
	append_once (StringList& files, String path)
	{
		for (StringList::iterator i = files.begin(); i != files.end(); ++i)
			if (*i == path)
				return;
		files.push_back(path);
	}
}

StringList
ManifestFile::get_files () const
{
	StringList files;

	String mpath = make_path(path, "manifest.txt");
	FILE* file = fopen(mpath, "r");
	if (file == NULL) {
		Log::Error << "Failed to open " << mpath << ": " << strerror(errno);
		return StringList();
	}

	char line[512];
	while (fgets(line, sizeof(line), file) != NULL) {
		// skip blank lines
		trim(line);
		if (line[0] == 0)
			continue;

		// don't allow directories or hidden files
		if (line[0] == '.' || strchr(line, '/') != NULL) {
			Log::Warning << "Ignoring file '" << line << "' in " << path << "/manifest.txt";
			continue;
		}

		// determine actual path
		String fpath = make_path(path, line);

		// make sure we have the proper extension
		if (!has_suffix(fpath, ext))
			continue;

		// fail if file does not exist
		if (!file_exists(fpath)) {
			Log::Warning << "File '" << line << "' listed in " << path << "/manifest.txt does not exist.";
			continue;
		}

		append_once(files, fpath);
	}

	return files;
}
