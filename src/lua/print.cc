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

#include "lua/core.h"
#include "lua/print.h"

// globals
namespace lua {
	extern lua_State* lua_state;
	IPrint* lua_print = NULL;
}

// the public interface
void lua::setPrint(IPrint* print)
{
	lua_print = print;
}

// this is our actual print function
namespace {
	int print(lua_State* s)
	{
		// check that we have a print object
		if (!lua::lua_print) {
			lua_pushstring(s, "print() is not allowed in this context");
			lua_error(s);
		}

		// print each argument to our printer
		for (int i = 1, e = lua_gettop(s); i != e; ++i) {
			size_t len;
			const char* str = lua_tolstring(s, i, &len);
			lua::lua_print->print(str, len);
		}

		// done; no return values
		return 0;
	}
}

// our initialization routine
namespace lua {
	bool initializePrint()
	{
		// register our print function as 'print'
		lua_register(lua_state, "print", print);
		return true;
	}
}
