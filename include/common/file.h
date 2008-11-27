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

namespace file {

// gets a list of all files in the requested path that
// have the requested extension.  does not search sub-
// directories, and never returns a file that starts with
// a .
StringList getFileList(std::string path, std::string ext);

} // namespace file

#endif
