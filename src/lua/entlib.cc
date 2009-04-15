/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "mud/entity.h"
#include "lua/core.h"
#include "lib/lua51/lua.h"
#include "lib/lua51/lauxlib.h"
#include "lib/lua51/lualib.h"

#define CHECKSELF(n) n* self = (n*)luaL_checkudata(s, 1, #n)

// -------------------
//   BINDINGS
// -------------------
namespace {
namespace bindings {

// -------------------
//   CLASS ENTITY
// -------------------
namespace entity {

int getName(lua_State*);
const luaL_Reg methods[] = {
	{ "getName", getName },
	{ NULL, NULL }
};

/**
 * name: Entity:getName
 * return: string
 */
int getName(lua_State* s)
{
	CHECKSELF(Entity);

	// push the name, return
	lua_pushstring(s, self->getName().getFull().c_str());
	return 1;
}

} // namespace entity

// -------------------
//   END BINDINGS
// -------------------

} // namespace bindings
} // anonymous

namespace Lua {
	bool initializeEntlib()
	{
		// create the Entity metatable
		luaL_newmetatable(Lua::state, "Entity");
		luaL_register(Lua::state, NULL, bindings::entity::methods);
		lua_pop(Lua::state, 1);

		return true;
	}
} // namespace Lua
