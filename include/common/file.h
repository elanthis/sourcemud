/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_FILE_H
#define SOURCEMUD_COMMON_FILE_H

#include <vector>
#include "common/string.h"

namespace File {

// gets a list of files in a particular directory.  does
// not return files that start with a . or end with a ~.
// if subdir is true, sub-directories will also be returned,
// otherwise only regular files are returned.
StringList dirlist(const std::string& path, bool subdir = false);

// filters the file list to only include files that match
// the given filter.  the filter is a UNIX-style shell glob,
// using fnmatch().
StringList& filter(StringList& files, const std::string& filter);

// returns true if the given path is a regular file
bool isfile(const std::string& path);

// deletes a file or an empty directory
void remove(const std::string& path);

// rename a file or directory, replacing target file
bool rename(const std::string& src, const std::string& dst);

} // namespace file

#endif
