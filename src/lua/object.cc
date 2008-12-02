/*
 * Source MUD
 * Copyright (C) 2008  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common/log.h"
#include "lua/object.h"
#include "lib/lua51/lua.h"
#include "lib/lua51/lauxlib.h"

namespace Lua {
	extern lua_State* state;
}

bool Lua::createObject(void* obj, const char* metatable)
{
	// create the userdata for the object
	void** data = (void**)lua_newuserdata(Lua::state, sizeof(void*));
	*data = obj;

	// look up the metatable
	luaL_getmetatable(Lua::state, metatable);
	if (lua_isnil(Lua::state, -1)) {
		// remove our user data and our nil
		lua_pop(Lua::state, 2);
		Log::Error << "Attempt to create Lua object with unknown "
			"metatable `" << metatable << "'";
		return false;
	}

	// set the metatable
	lua_setmetatable(Lua::state, -2);

	// this will be our index in the registry
	lua_pushlightuserdata(Lua::state, obj);

	// copy our object userdata
	lua_pushvalue(Lua::state, -2);

	// store in the registry
	lua_settable(Lua::state, LUA_REGISTRYINDEX);

	// remove the excess full userdata
	lua_pop(Lua::state, 1);

	return true;
}

void Lua::releaseObject(void* obj)
{
	// our key is a lightuserdata for the obj
	lua_pushlightuserdata(Lua::state, obj);

	// lookup the object from the registry
	lua_gettable(Lua::state, LUA_REGISTRYINDEX);

	// void the pointer in our userdata
	void** ptr = (void**)lua_touserdata(Lua::state, -1);
	if (ptr != NULL)
		*ptr = NULL;

	// remove the full user data
	lua_pop(Lua::state, 1);

	// push another light userdata key, and a nil value
	lua_pushlightuserdata(Lua::state, obj);
	lua_pushnil(Lua::state);

	// clear the registry
	lua_settable(Lua::state, LUA_REGISTRYINDEX);
}

void Lua::getObject(void* obj)
{
	// our key is a lightuserdata for the obj
	lua_pushlightuserdata(Lua::state, obj);

	// lookup the object from the registry
	lua_gettable(Lua::state, LUA_REGISTRYINDEX);
}
