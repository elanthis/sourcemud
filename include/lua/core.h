/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_LUA_CORE_H
#define SOURCEMUD_LUA_CORE_H

#include <string>

#include "common/imanager.h"

namespace lua {
	// Initialize the Lua subsystem
	bool initialize();

	// Close Lua
	void shutdown();

	// Load and execute a script file
	bool runfile(const std::string& path);
}

#endif
