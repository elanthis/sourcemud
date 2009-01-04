/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_FILE_H
#define SOURCEMUD_COMMON_FILE_H

namespace File
{

// gets a list of files in a particular directory.  does
// not return files that start with a . or end with a ~.
// if subdir is true, sub-directories will also be returned,
// otherwise only regular files are returned.
std::vector<std::string> dirlist(const std::string& path, bool subdir = false);

// filters the file list to only include files that match
// the given filter.  the filter is a UNIX-style shell glob,
// using fnmatch().
std::vector<std::string>& filter(std::vector<std::string>& files, const std::string& filter);

// returns true if the given path is a regular file
bool isfile(const std::string& path);

// deletes a file or an empty directory
void remove(const std::string& path);

// rename a file or directory, replacing target file
bool rename(const std::string& src, const std::string& dst);

// parse a path string into its components
struct pathinfo_t {
	std::string dir;
	std::string file;
	std::string ext;
};
pathinfo_t& pathinfo(const std::string& path, pathinfo_t& out);

// normalize a path (remove . and .. components, and add a leading /)
// this assumes that the path is meant to be absolute
std::string& normalize(std::string& path);

// get mime type (for now, just uses the extension)
const std::string& getMimeType(const std::string& path);

} // namespace file

#endif
