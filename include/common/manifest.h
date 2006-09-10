/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_COMMON_MANIFEST_H
#define AWEMUD_COMMON_MANIFEST_H

#include "common/gcvector.h"
#include "common/string.h"

StringList manifest_load (String path);
int manifest_save (String path, const StringList& files);

#endif
