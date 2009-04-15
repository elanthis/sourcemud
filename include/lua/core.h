/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_LUA_CORE_H
#define SOURCEMUD_LUA_CORE_H

#include "common.h"
#include "common/imanager.h"

struct lua_State;

namespace Lua {
	// Initialize the Lua subsystem
	bool initialize();

	// Close Lua
	void shutdown();

	// Load and execute a script file
	bool runfile(const std::string& path);

	// global stack (ick, but we need it)
	extern lua_State *state;
}

#endif
