/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include <dirent.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common/file.h"
#include "common/log.h"

StringList File::dirlist(const std::string& path, bool subdir)
{
	StringList list;

	// open our directory
	DIR* dir = opendir(path.c_str());
	if (dir == NULL) {
		Log::Error << "Failed to open directory: " << path;
		return list;
	}

	// read all entries
	dirent* d;
	while ((d = readdir(dir)) != NULL) {
		// skip entries with a leading .
		if (d->d_name[0] == '.')
			continue;

		// skip entries with a trailing ~
		if (d->d_name[strlen(d->d_name) - 1] == '~')
			continue;

		// store the file
		list.push_back(path + S("/") + std::string(d->d_name));

		// if subdir is off and we're not referencing a fail, skip it
		if (!subdir && !File::isfile(list.back())) {
			list.pop_back();
			continue;
		}
	}

	closedir(dir);
	return list;
}

StringList& File::filter(StringList& files, const std::string& filter)
{
	StringList::iterator i = files.begin();
	while (i != files.end()) {
		if (fnmatch(filter.c_str(), i->c_str(), 0) == 0)
			++i;
		else
			i = files.erase(i);
	}
	return files;
}

bool File::isfile(const std::string& path)
{
	struct stat st;
	int rs = stat(path.c_str(), &st);
	if (rs != 0)
		return false;
	if (!S_ISREG(st.st_mode))
		return false;
	return true;
}

void File::remove(const std::string& path)
{
	unlink(path.c_str());
}

bool File::rename(const std::string& src, const std::string& dst)
{
	return ::rename(src.c_str(), dst.c_str()) == 0;
}
