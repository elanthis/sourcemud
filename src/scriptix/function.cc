/*
 * Scriptix - Lite-weight scripting longerface
 * Copyright (c) 2002, 2003, 2004, 2005  Sean Middleditch
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

#include <string.h>
#include <iostream>

#include "scriptix/system.h"
#include "scriptix/function.h"
#include "scriptix/vimpl.h"
#include "common/strbuf.h"

using namespace Scriptix;

namespace {
	const int BLOCK_CHUNK = 64;

	class CatchHandler : public CompilerHandler {
		public:
		Scriptix::Function* function;

		CatchHandler () : function(NULL) {}

		virtual bool handle_function (Scriptix::Function* function, bool is_public) {
			CatchHandler::function = function;
			return true;
		}
	};
}

int
SScriptManager::add_function (Function* function)
{
	funcs[function->id] = function;
	return SXE_OK;
}

Function*
SScriptManager::get_function (Atom id)
{
	FunctionList::iterator i = funcs.find(id);
	if (i != funcs.end())
		return i->second;
	else
		return NULL;
}

Function::Function (Atom s_id, size_t s_argc) : IValue(),
	id(s_id), nodes(NULL), cfunc(NULL), file(), debug(NULL),
	argc(s_argc), varc(s_argc), count(0), size(0)
{
}

Function::Function (Atom s_id, size_t s_argc, sx_cfunc s_cfunc)
	: IValue(), id(s_id), nodes(NULL), cfunc(s_cfunc), file(), debug(NULL),
	argc(s_argc), varc(s_argc), count(0), size(0)
{
}

const TypeInfo*
Function::get_type () const {
	return ScriptManager.get_function_type();
}

int
Function::add_value (IValue* value) {
	intptr_t* sx_new_nodes;

	/* need at least two open places */
	if (size == 0 || count >= size - 1) {
		sx_new_nodes = (intptr_t*)GC_REALLOC (nodes, sizeof (long) * (size + BLOCK_CHUNK));
		if (sx_new_nodes == NULL)
			return SXE_NOMEM;
		nodes = sx_new_nodes;
		size += BLOCK_CHUNK;
	}
	nodes[count++] = OP_PUSH;
	nodes[count++] = (long)value;

	return SXE_OK;
}

int
Function::add_opcode (sx_op_type value) {
	intptr_t* sx_new_nodes;

	/* need at least two open places */
	if (size == 0 || count >= size - 1) {
		sx_new_nodes = (intptr_t*)GC_REALLOC (nodes, sizeof (long) * (size + BLOCK_CHUNK));
		if (sx_new_nodes == NULL)
			return SXE_NOMEM;
		nodes = sx_new_nodes;
		size += BLOCK_CHUNK;
	}
	nodes[count++] = value;

	return SXE_OK;
}

int
Function::add_oparg (long value) {
	intptr_t* sx_new_nodes;

	/* need at least two open places */
	if (size == 0 || count >= size - 1) {
		sx_new_nodes = (intptr_t*)GC_REALLOC (nodes, sizeof (long) * (size + BLOCK_CHUNK));
		if (sx_new_nodes == NULL)
			return SXE_NOMEM;
		nodes = sx_new_nodes;
		size += BLOCK_CHUNK;
	}
	nodes[count++] = value;

	return SXE_OK;
}

size_t
Function::get_line_of (size_t op_ptr) const {
	// no function body?
	if (count == 0)
		return 0;

	// bounds-check
	if (op_ptr >= count)
		op_ptr = count - 1;

	// loop until we find op code's line
	const DebugMetaData* d_ptr = debug;
	for (size_t c_op = d_ptr->span; op_ptr >= c_op; c_op += d_ptr->span)
		++d_ptr;
	return d_ptr->line;
}

ScriptFunction
ScriptFunction::compile (String name, String code, String args, String filename, unsigned long fileline)
{
	StringBuffer cname;
	for (String::iterator i = name.begin(); i != name.end(); ++i) {
		if (isalnum(*i) || *i == '_')
			cname << *i;
		else if (isspace(*i))
			cname << '_';
	}
	String source = S("function ") + cname.str() + S(" (") + args + S(")\n") + code + S("\nend");

	CatchHandler handler;

	// compile
	if (ScriptManager.load_string(source, filename, fileline, &handler) == Scriptix::SXE_OK)
		return handler.function;

	return ScriptFunction();
}

ScriptFunctionSource
ScriptFunctionSource::compile (String name, String code, String args, String filename, unsigned long fileline)
{
	return ScriptFunctionSource(ScriptFunction::compile(name, code, args, filename, fileline), code);
}

Scriptix::Value
ScriptFunction::run(size_t argc, Scriptix::Value argv[]) const {
	Scriptix::Value ret;
	if (Scriptix::ScriptManager.invoke(func, argc, argv, &ret) != Scriptix::SXE_OK)
		return Scriptix::Value();
	return ret;
}

Scriptix::Value
ScriptFunction::run() const {
	Scriptix::Value ret;
	if (Scriptix::ScriptManager.invoke(func, 0, NULL, &ret) != Scriptix::SXE_OK)
		return Scriptix::Value();
	return ret;
}

Scriptix::Value
ScriptFunction::run(Scriptix::Value a) const {
	Scriptix::Value ret;
	Scriptix::Value argv[1];
	argv[0] = a;
	if (Scriptix::ScriptManager.invoke(func, (sizeof(argv)/sizeof(argv[0])), argv, &ret) != Scriptix::SXE_OK)
		return Scriptix::Value();
	return ret;
}

Scriptix::Value
ScriptFunction::run(Scriptix::Value a, Scriptix::Value b) const {
	Scriptix::Value ret;
	Scriptix::Value argv[2];
	argv[0] = a;
	argv[1] = b;
	if (Scriptix::ScriptManager.invoke(func, (sizeof(argv)/sizeof(argv[0])), argv, &ret) != Scriptix::SXE_OK)
		return Scriptix::Value();
	return ret;
}

Scriptix::Value
ScriptFunction::run(Scriptix::Value a, Scriptix::Value b, Scriptix::Value c) const {
	Scriptix::Value ret;
	Scriptix::Value argv[3];
	argv[0] = a;
	argv[1] = b;
	argv[2] = c;
	if (Scriptix::ScriptManager.invoke(func, (sizeof(argv)/sizeof(argv[0])), argv, &ret) != Scriptix::SXE_OK)
		return Scriptix::Value();
	return ret;
}

Scriptix::Value
ScriptFunction::run(Scriptix::Value a, Scriptix::Value b, Scriptix::Value c, Scriptix::Value d) const {
	Scriptix::Value ret;
	Scriptix::Value argv[4];
	argv[0] = a;
	argv[1] = b;
	argv[2] = c;
	argv[3] = d;
	if (Scriptix::ScriptManager.invoke(func, (sizeof(argv)/sizeof(argv[0])), argv, &ret) != Scriptix::SXE_OK)
		return Scriptix::Value();
	return ret;
}

Scriptix::Value
ScriptFunction::run(Scriptix::Value a, Scriptix::Value b, Scriptix::Value c, Scriptix::Value d, Scriptix::Value e) const {
	Scriptix::Value ret;
	Scriptix::Value argv[5];
	argv[0] = a;
	argv[1] = b;
	argv[2] = c;
	argv[3] = d;
	argv[4] = e;
	if (Scriptix::ScriptManager.invoke(func, (sizeof(argv)/sizeof(argv[0])), argv, &ret) != Scriptix::SXE_OK)
		return Scriptix::Value();
	return ret;
}

Scriptix::Value
ScriptFunction::run(Scriptix::Value a, Scriptix::Value b, Scriptix::Value c, Scriptix::Value d, Scriptix::Value e, Scriptix::Value f) const {
	Scriptix::Value ret;
	Scriptix::Value argv[6];
	argv[0] = a;
	argv[1] = b;
	argv[2] = c;
	argv[3] = d;
	argv[4] = e;
	argv[5] = f;
	if (Scriptix::ScriptManager.invoke(func, (sizeof(argv)/sizeof(argv[0])), argv, &ret) != Scriptix::SXE_OK)
		return Scriptix::Value();
	return ret;
}

Scriptix::Value
ScriptFunction::run(Scriptix::Value a, Scriptix::Value b, Scriptix::Value c, Scriptix::Value d, Scriptix::Value e, Scriptix::Value f, Scriptix::Value g) const {
	Scriptix::Value ret;
	Scriptix::Value argv[7];
	argv[0] = a;
	argv[1] = b;
	argv[2] = c;
	argv[3] = d;
	argv[4] = e;
	argv[5] = f;
	argv[6] = g;
	if (Scriptix::ScriptManager.invoke(func, (sizeof(argv)/sizeof(argv[0])), argv, &ret) != Scriptix::SXE_OK)
		return Scriptix::Value();
	return ret;
}

Scriptix::Value
ScriptFunction::run(Scriptix::Value a, Scriptix::Value b, Scriptix::Value c, Scriptix::Value d, Scriptix::Value e, Scriptix::Value f, Scriptix::Value g, Scriptix::Value h) const {
	Scriptix::Value ret;
	Scriptix::Value argv[8];
	argv[0] = a;
	argv[1] = b;
	argv[2] = c;
	argv[3] = d;
	argv[4] = e;
	argv[5] = f;
	argv[6] = g;
	argv[7] = h;
	if (Scriptix::ScriptManager.invoke(func, (sizeof(argv)/sizeof(argv[0])), argv, &ret) != Scriptix::SXE_OK)
		return Scriptix::Value();
	return ret;
}

Scriptix::Value
ScriptFunction::run(Scriptix::Value a, Scriptix::Value b, Scriptix::Value c, Scriptix::Value d, Scriptix::Value e, Scriptix::Value f, Scriptix::Value g, Scriptix::Value h, Scriptix::Value i) const {
	Scriptix::Value ret;
	Scriptix::Value argv[9];
	argv[0] = a;
	argv[1] = b;
	argv[2] = c;
	argv[3] = d;
	argv[4] = e;
	argv[5] = f;
	argv[6] = g;
	argv[7] = h;
	argv[8] = i;
	if (Scriptix::ScriptManager.invoke(func, (sizeof(argv)/sizeof(argv[0])), argv, &ret) != Scriptix::SXE_OK)
		return Scriptix::Value();
	return ret;
}
