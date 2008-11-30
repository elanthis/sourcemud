/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include "lua/core.h"

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
 * name: setHook
 * param: string name
 * param: function callback
 * return: boid
 */
int setHook(lua_State* s)
{
	luaL_checkstring(s, 1);
	luaL_checktype(s, 2, LUA_TFUNCTION);

	// fetch our hook registry, or create it if it doesn't exist
	lua_getfield(s, LUA_REGISTRYINDEX, "_hooks");
	if (lua_isnil(s, -1)) {
		lua_newtable(s);
		lua_pushvalue(s, 1);
		lua_setfield(s, LUA_REGISTRYINDEX, "_hooks");
	}

	// rearrange fields (ugh)
	lua_pushvalue(s, 1);
	lua_pushvalue(s, 2);

	// set the hook
	lua_settable(s, 3);

	// return true value
	lua_pushboolean(s, true);
	return 1;
}

} // namespace bindings::mud

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

	return true;
}

}
