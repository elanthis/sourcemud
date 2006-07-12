/*
 * Scriptix - Lite-weight scripting interface
 * Copyright (c) 2002, 2003, 2004, 2005  AwesomePlay Productions, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef SCRIPTIX_FUNCTION_H
#define SCRIPTIX_FUNCTION_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common/gcbase.h"

#include "scriptix/value.h"
#include "scriptix/atom.h"
#include "scriptix/type.h"
#include "scriptix/opcodes.h"

namespace Scriptix {

struct DebugMetaData : public GCType::GC
{
	inline DebugMetaData (size_t s_span, size_t s_line) : span(s_span), line(s_line) {}

	size_t span;
	size_t line;
};

class Function : public IValue {
	public:
	Function ();
	Function (Atom id, size_t argc); // argc is minimum arg count
	Function (Atom id, size_t argc, sx_cfunc func); // argc is minimum arg count

	virtual const TypeInfo* get_type () const;

	// FIXME: should all be private
	Atom id; // name of function
	intptr_t* nodes; // byte codes
	sx_cfunc cfunc; // c function pointer (for cfuncs)
	String file; // file the function is implemented in
	DebugMetaData* debug; // debugging meta-data
	size_t argc; // number of arguments to function
	size_t varc; // number of variables in function
	size_t count; // number of valid bytecode nodes
	size_t size; // size of nodes

	// Build byte-codes
	public:
	int add_value (IValue* value);
	int add_opcode (sx_op_type code);
	int add_oparg (long arg);

	size_t get_line_of (size_t op_ptr) const;
};

// hold a script function that can be invoked
class ScriptFunction : public GCType::GC
{
	public:
	inline ScriptFunction (Scriptix::Function* s_func) : func(s_func) {}
	inline ScriptFunction (const ScriptFunction& s_func) : func(s_func.func) {}
	inline ScriptFunction () : func(NULL) {}

	inline bool empty () const { return func == NULL; }

	inline void clear () { func = NULL; }

	inline Scriptix::Function* get () const { return func; }
	inline operator Scriptix::Function* () const { return get(); }

	static ScriptFunction compile (String func_name, String code, String args, String filename, unsigned long fileline);

	Value run() const;
	Value run(Value a) const;
	Value run(Value a, Value b) const;
	Value run(Value a, Value b, Value c) const;
	Value run(Value a, Value b, Value c, Value d) const;
	Value run(Value a, Value b, Value c, Value d, Value e) const;
	Value run(Value a, Value b, Value c, Value d, Value e, Value f) const;
	Value run(Value a, Value b, Value c, Value d, Value e, Value f, Value g) const;
	Value run(Value a, Value b, Value c, Value d, Value e, Value f, Value g, Value h) const;
	Value run(Value a, Value b, Value c, Value d, Value e, Value f, Value g, Value h, Value i) const;

	private:
	Scriptix::Function* func;
};

class ScriptFunctionSource : public ScriptFunction
{
	public:
	inline ScriptFunctionSource () : ScriptFunction(), source() {}
	inline ScriptFunctionSource (const ScriptFunctionSource& s_func) : ScriptFunction(s_func), source(s_func.source) {}
	inline ScriptFunctionSource (const ScriptFunction& s_func, String s_source) : ScriptFunction(s_func), source(s_source) {}

	inline const String& get_source () const { return source; }

	inline void clear () { ScriptFunction::clear(); source.clear(); }

	static ScriptFunctionSource compile (String func_name, String code, String args, String filename, unsigned long fileline);

	private:
	String source;
};

} // namespace Scriptix

#endif
