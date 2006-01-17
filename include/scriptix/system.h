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

#ifndef SCRIPTIX_SYSTEM_H
#define SCRIPTIX_SYSTEM_H

#include "common/gcbase.h"
#include "common/gcvector.h"
#include "common/gcmap.h"
#include "common/imanager.h"

#include "scriptix/function.h"
#include "scriptix/type.h"
#include "scriptix/thread.h"
#include "scriptix/error.h"

namespace Scriptix {

class CompilerHandler : public GCType::GC {
	public:
	// return false to generate a compilation error
	virtual bool handle_function (Function* function, bool is_public) { return true; }
	virtual bool handle_global (Atom id, Value value, bool is_public) { return true; }
	virtual bool handle_class (TypeInfo* type) { return true; }

	virtual ~CompilerHandler () {}
};

class SScriptManager : public IManager {
	public:
	// setup/destroy
	int initialize ();
	void shutdown ();

	// Handle an error
	virtual void handle_error (const BaseString& file, size_t lineno, const BaseString& msg);

	// Add/check types
	TypeInfo* add_type (const TypeDef* type);
	const TypeInfo* get_type (Atom id) const;
	TypeInfo* get_type (Atom id);

	inline const TypeInfo* get_number_type () const { return t_number; }
	inline const TypeInfo* get_string_type () const { return t_string; }
	inline const TypeInfo* get_struct_type () const { return t_struct; }
	inline const TypeInfo* get_iterator_type () const { return t_iterator; }
	inline const TypeInfo* get_array_type () const { return t_array; }
	inline const TypeInfo* get_function_type () const { return t_function; }
	inline const TypeInfo* get_type_value_type () const { return t_type; }
	inline const TypeInfo* get_script_class_type () const { return t_script_class; }

	// Functions
	int add_function (Function* function);
	Function* get_function (Atom id);

	// Globals
	int add_global (Atom id, Value value);
	Value get_global (Atom id) const;

	// invoke a function
	int invoke (const Function* func, size_t argc, Value argv[], Value* retval);

	// invoke a method
	int invoke (Value self, Atom method, size_t argc, Value argv[], Value* retval);

	// Raise an error condition
	int raise_error (int err, const char *format, ...); // automatic file/line
	int raise_arg_error (const char* func, const char* arg, const char* type);

	// Load/compile scripts
	int load_file (const BaseString& filepath, CompilerHandler* handler = NULL);
	int load_string (const BaseString& buffer, const BaseString& name, size_t lineno = 1, CompilerHandler* handler = NULL);

	private:
	// Fetch stack item from end (args) - INLINE for speed
	inline Value get_value (size_t index) { return data_stack[data_stack.size() - index]; }
	// Same as get_value(1):
	inline Value get_value (void) { return data_stack.back(); }

	// Fetch item from frame stack for op atrgs; "eats" arg
	inline int get_oparg (void) { return get_frame().func->nodes[get_frame().op_ptr++]; }

	// Manipulate data stack - INLINE for speed
	inline int
	push_value (Value value) {
		data_stack.push_back(value);
		return SXE_OK;
	}
	inline void pop_value (size_t len = 1) { data_stack.resize(data_stack.size() - len); }

	// Manipulate data stack
	int push_frame (const Function* func, size_t argc, Value argv[]);
	void pop_frame (void);
	inline Frame& get_frame (void) { return frames.back(); }
	inline const Frame& get_frame (void) const { return frames.back(); }

	// runtime state
	int state;

	// function frame stack
	typedef GCType::vector<Frame> FrameStack;
	FrameStack frames;

	// data stack
	typedef GCType::vector<Value> DataStack;
	DataStack data_stack;

	// global data
	typedef GCType::map<Atom,Value> GlobalList;
	GlobalList globals;

	// public functions
	typedef GCType::map<Atom,Function*> FunctionList;
	FunctionList funcs;

	// types
	typedef GCType::map<Atom,TypeInfo*> TypeList;
	TypeList types;

	// built-in types
	const TypeInfo* t_value;
	const TypeInfo* t_number;
	const TypeInfo* t_string;
	const TypeInfo* t_array;
	const TypeInfo* t_function;
	const TypeInfo* t_type;
	const TypeInfo* t_struct;
	const TypeInfo* t_iterator;
	const TypeInfo* t_script_class;
};

extern SScriptManager ScriptManager;

} // namespace Scriptix

#endif
