/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common/log.h"
#include "lua/core.h"
#include "lib/lua51/lua.h"
#include "lib/lua51/lauxlib.h"
#include "lib/lua51/lualib.h"

// -------------------
//   BINDINGS
// -------------------
namespace {
namespace bindings {

// -------------------
//   MUD.*
// -------------------
namespace mud {

int setHook(lua_State*);
const luaL_Reg registry[] = {
	{ "setHook", setHook },
	{ NULL, NULL }
};

/**
 * name: mud.setHook
 * param: string name
 * param: function callback
 */
int setHook(lua_State* s)
{
	luaL_checkstring(s, 1);
	luaL_checktype(s, 2, LUA_TFUNCTION);

	// fetch our hook registry, or create it if it doesn't exist
	lua_getfield(s, LUA_REGISTRYINDEX, "_hooks");
	if (lua_isnil(s, -1)) {
		lua_newtable(s);
		lua_pushvalue(s, -1);
		lua_setfield(s, LUA_REGISTRYINDEX, "_hooks");
	}

	// rearrange fields (ugh)
	lua_pushvalue(s, 1);
	lua_pushvalue(s, 2);

	// set the hook
	lua_settable(s, -3);

	// return true value
	lua_pushboolean(s, true);
	return 1;
}

} // namespace bindings::mud

// -------------------
//   LOG.*
// -------------------
namespace log {

int info(lua_State*);
int error(lua_State*);
int warning(lua_State*);
const luaL_Reg registry[] = {
	{ "info", info},
	{ NULL, NULL }
};

/**
 * name: log.info
 * param: string ...
 */
int info(lua_State* s)
{
	const StreamControl& st(Log::Info);
	st << "LUA: ";
	for (int i = 1, e = lua_gettop(s); i <= e; ++i)
		st << lua_tostring(s, i);
	return 0;
}

/**
 * name: log.error
 * param: string ...
 */
int error(lua_State* s)
{
	const StreamControl& st(Log::Error);
	st << "LUA: ";
	for (int i = 1, e = lua_gettop(s); i <= e; ++i)
		st << lua_tostring(s, i);
	return 0;
}

/**
 * name: log.warning
 * param: string ...
 */
int warning(lua_State* s)
{
	const StreamControl& st(Log::Warning);
	st << "LUA: ";
	for (int i = 1, e = lua_gettop(s); i <= e; ++i)
		st << lua_tostring(s, i);
	return 0;
}

} // namespace bindings::log

// -------------------
//   END BINDINGS
// -------------------

} // namespace bindings
} // anonymous

namespace Lua {

extern lua_State* state;

bool initializeMudlib()
{
	luaL_register(Lua::state, "mud", bindings::mud::registry);
	luaL_register(Lua::state, "log", bindings::log::registry);
	return true;
}

}
