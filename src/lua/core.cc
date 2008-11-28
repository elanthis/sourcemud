/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "common/log.h"
#include "lua/core.h"

namespace lua {
	lua_State* lua_state = NULL;

	extern bool initializePrint();
}

bool lua::initialize()
{
	// initialize only once
	if (lua_state != NULL)
		return true;

	Log::Info << "Initializing Lua...";

	// initialize the Lua state object
	if ((lua_state = luaL_newstate()) == NULL) {
		Log::Error << "luaL_newstate() failed";
		return false;
	}

	// load Lua libs
	luaL_openlibs(lua_state);

	// initialize our custom libraries
	if (!initializePrint())
		return false;

	return true;
}

void lua::shutdown()
{
	if (lua_state != NULL) {
		lua_close(lua_state);
		lua_state = NULL;
	}
}

bool lua::runfile(const std::string& path)
{
	if (!lua_state) {
		Log::Error << "lua::runfile(): Lua not initialized";
		return false;
	}

	// load the script
	int rs = luaL_loadfile(lua_state, path.c_str());
	if (rs != 0) {
		Log::Error << "Couldn't load script: " << lua_tostring(lua_state, -1);
		lua_pop(lua_state, 1);
		return false;
	}
	
	// execute the script
	rs = lua_pcall(lua_state, 0, 0, 0);
	if (rs != 0) {
		Log::Error << "Couldn't execute script: " << lua_tostring(lua_state, -1);
		lua_pop(lua_state, 1);
		return false;
	}

	return true;
}
