/*
 * Source MUD
 * Copyright (C) 2008  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/log.h"
#include "lua/core.h"
#include "lua/exec.h"
#include "lua/print.h"
#include "lib/lua51/lua.h"
#include "lib/lua51/lauxlib.h"
#include "lib/lua51/lualib.h"

Lua::Exec::Exec(const std::string& funcname) : stack(0), print(NULL)
{
	// look up the function
	lua_getglobal(Lua::state, funcname.c_str());
	if (!lua_isfunction(Lua::state, -1))
		lua_pop(Lua::state, 1);
	else
		stack = 1;
}

void Lua::Exec::param(bool v)
{
	if (stack != 0) {
		lua_pushboolean(Lua::state, v);
		++stack;
	}
}

void Lua::Exec::param(double v)
{
	if (stack != 0) {
		lua_pushnumber(Lua::state, v);
		++stack;
	}
}

void Lua::Exec::param(const char* str, size_t len)
{
	if (stack != 0) {
		lua_pushlstring(Lua::state, str, len);
		++stack;
	}
}

void Lua::Exec::param(void *object) {
	if (stack != 0) {
		Lua::getObject(object);
		++stack;
	}
}

void Lua::Exec::table()
{
	if (stack != 0) {
		lua_newtable(Lua::state);
		++stack;
	}
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
	return stack != 0 && lua_isboolean(Lua::state, -1);
}

bool Lua::Exec::isNumber()
{
	return stack != 0 && lua_isnumber(Lua::state, -1);
}

bool Lua::Exec::isString()
{
	return stack != 0 && lua_isstring(Lua::state, -1);
}

bool Lua::Exec::getBoolean()
{
	return stack != 0 ? lua_toboolean(Lua::state, -1) : false;
}

double Lua::Exec::getNumber()
{
	return stack != 0 ? lua_tonumber(Lua::state, -1) : 0.0;
}

std::string Lua::Exec::getString()
{
	return stack != 0 ? lua_tostring(Lua::state, -1) : std::string();
}

bool Lua::Exec::run()
{
	// no function, nothing to invoke
	if (stack == 0)
		return false;

	// set the print handler
	Lua::setPrint(print);

	// execute the function
	if (lua_pcall(Lua::state, stack - 1, 1, 0) != 0) {
		Log::Info << "post-run " << lua_gettop(Lua::state) << " " << stack;
		// remove the error
		lua_pop(Lua::state, 1);
		// we have nothing left on the stack
		stack = 0;
		// clear the print handler
		Lua::setPrint(NULL);
		return false;
	} else {
		// one and only one return value
		stack = 1;
		// clear the print handler
		Lua::setPrint(NULL);
		return true;
	}
}

Lua::ExecHook::ExecHook(const std::string& hookname) : Lua::Exec()
{
	// look up hook table
	lua_getfield(Lua::state, LUA_REGISTRYINDEX, "_hooks");
	if (!lua_isnil(Lua::state, -1)) {
		// get the hook from the table
		lua_getfield(Lua::state, -1, hookname.c_str());
		if (!lua_isnil(Lua::state, -1)) {
			// remove hook table
			lua_remove(Lua::state, -2);
			stack = 1;
		} else {
			// clean up table and nil
			lua_pop(Lua::state, 2);
		}
	} else {
		// clean up table
		lua_pop(Lua::state, 1);
	}
}

void Lua::Exec::cleanup()
{
	lua_pop(Lua::state, stack);
	stack = 0;
}
