/*
 * Source MUD
 * Copyright (C) 2008  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

extern "C" {
#include <lua.h>
}

#include "common/log.h"
#include "lua/core.h"
#include "lua/exec.h"
#include "lua/print.h"

namespace Lua {
	extern lua_State* state;
}

void Lua::Exec::param(bool v)
{
	lua_pushboolean(Lua::state, v);
	++stack;
}

void Lua::Exec::param(double v)
{
	lua_pushnumber(Lua::state, v);
	++stack;
}

void Lua::Exec::param(const char* str, size_t len)
{
	lua_pushlstring(Lua::state, str, len);
	++stack;
}

void Lua::Exec::table()
{
	lua_newtable(Lua::state);
	++stack;
}

void Lua::Exec::setTable(const char* key)
{
	// only try if we have at least 2 elements, and the second-to-top
	// element is a table
	if (stack > 1 && lua_istable(Lua::state, -2)) {
		lua_setfield(Lua::state, -2, key);
		--stack;
	}
}

bool Lua::Exec::isBoolean()
{
	return lua_isboolean(Lua::state, -1);
}

bool Lua::Exec::isNumber()
{
	return lua_isnumber(Lua::state, -1);
}

bool Lua::Exec::isString()
{
	return lua_isstring(Lua::state, -1);
}

bool Lua::Exec::getBoolean()
{
	return lua_toboolean(Lua::state, -1);
}

double Lua::Exec::getNumber()
{
	return lua_tonumber(Lua::state, -1);
}

std::string Lua::Exec::getString()
{
	return lua_tostring(Lua::state, -1);
}

bool Lua::Exec::run(const std::string& name)
{
	// look up the function
	lua_getglobal(Lua::state, name.c_str());
	if (lua_isnil(Lua::state, -1)) {
		lua_pop(Lua::state, 1);
		return false;
	}

	// execute the function
	if (lua_pcall(Lua::state, stack, 1, 0) != 0) {
		// just the error is left
		stack = 1;
		// log error
		Log::Error << "Error executing `" << name << "': "
			<< lua_tostring(Lua::state, -1);
		return false;
	} else {
		// one and only one return value
		stack = 1;
		return true;
	}
}

bool Lua::Exec::runHook(const std::string& name)
{
	// look up hook table
	lua_getfield(Lua::state, LUA_REGISTRYINDEX, "_hooks");
	if (lua_isnil(Lua::state, -1)) {
		lua_pop(Lua::state, 1);
		return false;
	}

	// get the hook from the table
	lua_getfield(Lua::state, -1, name.c_str());
	if (lua_isnil(Lua::state, -1)) {
		lua_pop(Lua::state, 2);
		return false;
	}
	lua_replace(Lua::state, -2);

	// set the print handler
	Lua::setPrint(print);

	// execute the function
	if (lua_pcall(Lua::state, stack, 1, 0) != 0) {
		// just the error is left
		stack = 1;
		// note that we received an error
		error = true;
		// clear print handler
		Lua::setPrint(print);
		// log error
		Log::Error << "Error executing `" << name << "': "
			<< lua_tostring(Lua::state, -1);
		return false;
	} else {
		// one and only one return value
		stack = 1;
		// clear print handler
		Lua::setPrint(print);
		return true;
	}
}

void Lua::Exec::cleanup()
{
	lua_pop(Lua::state, stack);
	stack = 0;
	error = false;
}
