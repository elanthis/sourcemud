/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/log.h"
#include "lua/core.h"
#include "lib/lua51/lua.h"
#include "lib/lua51/lauxlib.h"
#include "lib/lua51/lualib.h"

namespace Lua
{
	lua_State *state = NULL;

	extern bool initializePrint();
	extern bool initializeMisclib();
	extern bool initializeMudlib();
	extern bool initializeEntlib();
}

bool Lua::initialize()
{
	// initialize only once
	if (state != NULL)
		return true;

	Log::Info << "Initializing Lua...";

	// initialize the Lua state object
	if ((Lua::state = luaL_newstate()) == NULL) {
		Log::Error << "luaL_newstate() failed";
		return false;
	}

	// load Lua libs
	luaL_openlibs(state);

	// set some special variables
	lua_pushstring(state, PACKAGE_VERSION);
	lua_setglobal(state, "VERSION");

	lua_pushstring(state, __DATE__ " " __TIME__);
	lua_setglobal(state, "BUILD");

	// initialize our custom libraries
	if (!initializePrint())
		return false;
	if (!initializeMisclib())
		return false;
	if (!initializeMudlib())
		return false;
	if (!initializeEntlib())
		return false;

	return true;
}

void Lua::shutdown()
{
	if (state != NULL) {
		lua_close(state);
		state = NULL;
	}
}

bool Lua::runfile(const std::string& path)
{
	if (!state) {
		Log::Error << "Lua::runfile(): Lua not initialized";
		return false;
	}

	// load the script
	int rs = luaL_loadfile(state, path.c_str());
	if (rs != 0) {
		Log::Error << "Couldn't load script: " << lua_tostring(state, -1);
		lua_pop(state, 1);
		return false;
	}

	// execute the script
	rs = lua_pcall(state, 0, 0, 0);
	if (rs != 0) {
		Log::Error << "Couldn't execute script: " << lua_tostring(state, -1);
		lua_pop(state, 1);
		return false;
	}

	return true;
}
