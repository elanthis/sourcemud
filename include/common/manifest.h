/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_MANIFEST_H
#define SOURCEMUD_COMMON_MANIFEST_H

#include <vector>
#include "common/string.h"

class ManifestFile
{
	public:
	ManifestFile (String s_path, String s_ext) : path(s_path), ext(s_ext) {}

	StringList get_files () const;
	bool has_file (String file) const;
	int remove_file (String file);
	int add_file (String file);

	private:
	String path;
	String ext;
};

#endif
