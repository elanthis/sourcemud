/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/file.h"
#include "common/log.h"
#include "common/string.h"

std::vector<std::string> File::dirlist(const std::string& path, bool subdir)
{
	std::vector<std::string> list;

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
		list.push_back(path + "/" + std::string(d->d_name));

		// if subdir is off and we're not referencing a fail, skip it
		if (!subdir && !File::isfile(list.back())) {
			list.pop_back();
			continue;
		}
	}

	closedir(dir);
	return list;
}

std::vector<std::string>& File::filter(std::vector<std::string>& files, const std::string& filter)
{
	std::vector<std::string>::iterator i = files.begin();
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

File::pathinfo_t& File::pathinfo(const std::string& path, pathinfo_t& info)
{
	// search for the final directory separator, if any, and
	// split the file and directory bits
	size_t dsep = path.rfind('/');
	if (dsep != std::string::npos) {
		info.dir = path.substr(0, dsep);
		info.file = path.substr(dsep + 1);
	} else {
		info.dir = ".";
		info.file = path;
	}

	// look for the extension, if any
	size_t esep = info.file.rfind('.');
	if (esep != std::string::npos)
		info.ext = info.file.substr(esep + 1);
	else
		info.ext.clear();

	return info;
}

std::string& File::normalize(std::string& path)
{
	// FIXME: this is not exactly a speedy way of doing this.  but it
	// does work quite well.  a more efficient implementation is definitely
	// possible without _too_ much extra effort

	// break the path into components
	std::vector<std::string> parts;
	explode(parts, path, '/');

	// look at each component, remove/keep as appropriate
	std::vector<std::string>::iterator i = parts.begin();
	while (i != parts.end()) {
		// remove . components
		if (*i == ".") {
			i = parts.erase(i);
			// ignore empty components
		} else if (i->empty()) {
			i = parts.erase(i);
			// for .. component, remove both it and the previous component
		} else if (*i == "..") {
			i = parts.erase(i);
			if (i != parts.begin())
				i = parts.erase(--i);
			// keep everything else
		} else {
			++i;
		}
	}

	// now we rejoin all components, and we also add in a leading /
	path = "/" + implode(parts, '/');
	return path;
}

const std::string& File::getMimeType(const std::string& path)
{
	// mime types mapping
	static std::map<std::string, std::string> mime_map;
	static std::string mime_unknown = "unknown";
	if (mime_map.empty()) {
		mime_map["txt"] = "text/plain";
		mime_map["html"] = "text/html";
		mime_map["css"] = "text/css";
		mime_map["js"] = "text/js";
		mime_map["lua"] = "application/x-lua";
		mime_map["png"] = "image/png";
		mime_map["jpg"] = "image/jpg";
		mime_map["gif"] = "image/gif";
	}

	// get the file's extension
	size_t esep = path.find_last_of("/.");
	if (esep == std::string::npos)
		return mime_unknown;

	// copy and lower-case the string
	std::string ext = strlower(path.substr(esep + 1));

	// find the extension in the map, or return unknown if not found
	std::map<std::string, std::string>::iterator i = mime_map.find(ext);
	if (i != mime_map.end())
		return i->second;
	else
		return mime_unknown;
}
