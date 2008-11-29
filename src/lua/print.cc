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
namespace Lua {
	extern lua_State* state;
	IStreamSink* sink;
}

// the public interface
void Lua::setPrint(IStreamSink* sink)
{
	Lua::sink = sink;
}

// this is our actual print function
namespace {
	int do_print(lua_State* s)
	{
		// check that we have a stream sink
		if (!Lua::sink) {
			lua_pushstring(s, "print() is not allowed in this context");
			lua_error(s);
		}

		// print each argument to our printer
		for (int i = 1, e = lua_gettop(s); i <= e; ++i) {
			size_t len;
			const char* str = lua_tolstring(s, i, &len);
			Lua::sink->stream_put(str, len);
		}

		// done; no return values
		return 0;
	}
}

// our initialization routine
namespace Lua {
	bool initializePrint()
	{
		// register our print function as 'print'
		lua_register(Lua::state, "print", do_print);
		return true;
	}
}
