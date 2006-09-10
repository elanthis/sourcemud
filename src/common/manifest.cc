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

	void
	do_wildcard (StringList& files, String path, const char* wild)
	{
		DIR* dir;
		dirent* d;

		dir = opendir(path);
		if (dir == NULL) {
			Log::Error << "Failed to read directory " << path << ": " << strerror(errno);
			return;
		}

		while ((d = readdir(dir)) != NULL) {
			// ignore dot files
			if (d->d_name[0] == '.')
				continue;

			// if doesn't match, skip it
			if (fnmatch(wild, d->d_name, 0) != 0)
				continue;

			// determine actual path
			String fpath = make_path(path, d->d_name);

			// we know it exists, but make sure it's a regular file
			if (!file_exists(fpath))
				continue;

			append_once(files, fpath);
		}

		closedir(dir);
	}
}

StringList
manifest_load (String path)
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

		// don't allow directories
		if (line[0] == '.' || strchr(line, '/') != NULL) {
			Log::Warning << "Ignoring file '" << line << "' in " << path << "/manifest.txt";
			continue;
		}

		// wildcard?
		if (strpbrk(line, "*?[") != NULL) {
			// process wildcard
			do_wildcard(files, path, line);
			continue;
		}

		// determine actual path
		String fpath = make_path(path, line);

		// fail if file does not exist
		if (!file_exists(fpath)) {
			Log::Warning << "File '" << line << "' listed in " << path << "/manifest.txt does not exist.";
			continue;
		}

		append_once(files, fpath);
	}

	Log::Info << mpath << ":";
	for (StringList::iterator i = files.begin(); i != files.end(); ++i)
		Log::Info << "  " << *i;

	return files;
}

int
manifest_save (String path, const StringList& files)
{
	String mpath = make_path(path, "manifest.txt");
	FILE* file = fopen(mpath, "wt");
	if (file == NULL) {
		Log::Error << "Failed to write to " << mpath << ": " << strerror(errno);
		return -1;
	}

	for (StringList::const_iterator i = files.begin(); i != files.end(); ++i) {
		fputs(*i, file);
		putc('\n', file);
	}

	fclose(file);

	return 0;
}
