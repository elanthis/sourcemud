/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include <dirent.h>

#include "common/file.h"
#include "common/log.h"

StringList file::getFileList(std::string path, std::string ext)
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

		// length must be long enough for the extension and a .
		// and an actual name
		if (strlen(d->d_name) < ext.size() + 2)
			continue;

		// must be a . before the extension
		if (d->d_name[strlen(d->d_name) - ext.size() - 1] != '.')
			continue;

		// and of course must have the actual extension
		if (ext != &d->d_name[strlen(d->d_name) - ext.size()])
			continue;

		// add file to our result list!
		list.push_back(path + S("/") + std::string(d->d_name));
	}

	closedir(dir);
	return list;
}
