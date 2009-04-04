/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "mud/entity.h"
#include "mud/fileobj.h"
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
namespace misc {

// ---------------------
//   CLASS FILEWRITER
// ---------------------
namespace filewriter {

int attr(lua_State*);
const luaL_Reg methods[] = {
	{ "attr", attr },
	{ NULL, NULL }
};

/**
 * name: FileWriter:attr
 * param: name string
 * param: value mixed 
 * return: string
 */
int attr(lua_State* s)
{
	CHECKSELF(File::Writer);
	std::string name = luaL_checkstring(s, 1);
	luaL_checkany(s, 2);

	// determine type of value
	switch (lua_type(s, 2)) {
	case LUA_TNIL:
		/* ignore */
		break;
	case LUA_TBOOLEAN:
		self->attr(name, "userdata", (bool)lua_toboolean(s, 2));
		break;
	case LUA_TNUMBER:
		self->attr(name, "userdata", lua_tointeger(s, 2));
		break;
	case LUA_TSTRING:
		self->attr(name, "userdata", std::string(lua_tostring(s, 2)));
		break;
	}

	return 0;
}

} // namespace filewriter

// -------------------
//   END BINDINGS
// -------------------

} // namespace misc
} // namespace bindings
} // anonymous

namespace Lua {

extern lua_State* state;

bool initializeMisclib()
{
	// create the Entity metatable
	luaL_newmetatable(Lua::state, "FileWriter");
	luaL_register(Lua::state, NULL, bindings::misc::filewriter::methods);
	lua_pop(Lua::state, 1);

	return true;
}

} // namespace Lua
