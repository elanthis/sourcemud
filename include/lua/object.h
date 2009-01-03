/*
 * Source MUD
 * Copyright (C) 2008  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 *
 * OBJECT MANAGER
 * Allows for creating userdata objects with a specific metatable, and
 * referencing/unreferencing/fetching them.
 *
 * Used to wrap C++ classes for Lua.
 *
 * Our wrapper classes allow for the underlying object to be NULLed out,
 * so when the C++ code decides the object is gone, it can safely free the
 * memory without worrying about Lua.
 */

#ifndef SOURCEMUD_LUA_OBJECT_H
#define SOURCEMUD_LUA_OBJECT_H

#include <string>

namespace Lua
{

// creates a new user data that contains the provided pointer,
// using the metatable with the given name.  The userdata is
// registered in the Lua registry to prevent collection.  The
// userdata is also added to the stack.  Each object (pointer
// address) can have only one Lua object at a time.
// returns true if the metatable was found, and false otherwise.
// no object is left on the stack.
bool createObject(void* obj, const char* metatable);

// releases the given object from the Lua registry, allowing
// it to be collected.  The userdata's pointer is also NULLed
// out, so that the real object can be safely freed without
// worrying about Lua scripts trying to access it.
void releaseObject(void* obj);

// pushes the Lua userdata for the specified object onto the
// Lua stack.  if no userdata can be found, nil is pushed
// instead.
void getObject(void* obj);

}

#endif
