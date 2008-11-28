/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 *
 * LUA PRINT HANDLER
 * This facility allows for directing where the global print() function
 * in Lua scripts will send its output.  By default there is no print
 * handler, so any calls to print() will result in an error.
 *
 * This can be over-ridden as necessary.  For example, the HTTP subsystem
 * would redirect printing to send data to the client.  A room description
 * handler might direct printing to the output stream for the character
 * viewing the room.
 *
 * Calling lua::setPrint() will set a new handler, but it will not free
 * the handler.  It is best to create a subclass of IPrint on the stack,
 * pass it to lua::setPrint(), invoke the necessary Lua scripts, and then
 * immediately call lua::clearPrint().  This guarantees that no objects
 * are leaked and that printing keeps working as intended.
 */

#ifndef SOURCEMUD_LUA_PRINT_H
#define SOURCEMUD_LUA_PRINT_H

namespace lua {
	// A print handling callback
	class IPrint {
		public:
		virtual ~IPrint() {}
		virtual void print(const char* bytes, size_t len) = 0;
	};

	// Set the current print handler
	void setPrint(IPrint*);

	// Clear the print handler
	void clearPrint() { setPrint(NULL); }
}

#endif
