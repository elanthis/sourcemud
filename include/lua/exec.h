/*
 * Source MUD
 * Copyright (C) 2008  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 *
 * LUA EXECUTION HELPER
 * This class is a wrapper that makes it slighty easier to execute Lua
 * functions from C++ code.  The intent is to make it simple to build up
 * a list of arguments, execute the desired function, and then retrieve
 * a single return value.  It also ensures that the stack is kept nice
 * and tidy.
 *
 * To add arguments, use the param() methods.  To execute a function, use
 * one of the run*() methods.  The run() method will return true if the
 * function ran and false is the function could not be executed or if an
 * error occured during execution.
 *
 * isValid() returns true if the function was successfully found.  It
 * MUST be called before setting any parameters, or else it will always
 * return true.
 *
 * The return value can be retrieved using the get*() functions, and
 * checked using the is*() functions.
 *
 * Note that this really is just a thin wrapper over the Lua stack
 * manipulation functions.  While it tries to Do The Right Thing(tm), it
 * can be confused if misused or abused.
 *
 * The setPrint() method is used to set the output handler used for the
 * global print function in Lua.  This guarantees that it is unset after
 * the function is complete.  When complete, the print handler is cleared;
 * it is NOT restored to the previous value, if any.
 *
 * A single Exec object may be used for multiple function calls.  However,
 * the arguments are not kept between calls.  The cleanup() MUST be called
 * before reusing an Exec object.
 *
 * ExecHook is a sub-class that looks up a hook function set with the
 * mud.setHook function in Lua.
 */

#ifndef SOURCEMUD_LUA_EXEC_H
#define SOURCEMUD_LUA_EXEC_H

#include <string>

#include "common/streams.h"

namespace Lua
{

// Execution manager
class Exec
{
public:
	// pass in the name of the function to be called
	Exec(const std::string& funcname);
	~Exec() { cleanup(); }

	// returns true if a function was successfully set.
	// MUST be called before setting parameters, or it will lie.
	bool isValid() { return stack; }

	// boolean parameter
	void param(bool);

	// numeric parameters
	void param(double);
	void param(float v) { param((double)v); }
	void param(signed char v) { param((double)v); }
	void param(unsigned char v) { param((double)v); }
	void param(signed short v) { param((double)v); }
	void param(unsigned short v) { param((double)v); }
	void param(signed long v) { param((double)v); }
	void param(unsigned long v) { param((double)v); }
	void param(signed long long v) { param((double)v); }
	void param(unsigned long long v) { param((double)v); }

	// string parameters
	void param(const char* str, size_t len);
	void param(const char* v) { param(v, strlen(v)); }
	void param(const std::string& v) { param(v.c_str(), v.size()); }

	// create a table, or set a table element
	void table();
	void setTable(const char*);
	void setTable(const std::string& s) { setTable(s.c_str()); }

	template <typename T> void setTable(const char* s, T v) {
		param(v);
		setTable(s);
	}

	// check return value types
	bool isBoolean();
	bool isNumber();
	bool isString();

	// get return value
	bool getBoolean();
	double getNumber();
	int getInteger() { return getNumber(); }
	std::string getString();

	// executes the function
	bool run();

	// set our print handler
	void setPrint(IStreamSink* stream) { print = stream; }

	// pop any values from the stack; automatically done by the
	// destructor.  this is used if you want to bail out early
	// but the Exec object isn't going out of scope.
	void cleanup();

protected:
	// tracks how many items are on the stack that we need to pop
	// we destructed.
	size_t stack;
	// passed to Lua::setPrint() when run() is called.
	IStreamSink* print;

	// just for ExecHook
protected:
	Exec() : stack(0), print(NULL) {}

};

class ExecHook : public Exec
{
public:
	ExecHook(const std::string& hookname);
};

}

#endif
