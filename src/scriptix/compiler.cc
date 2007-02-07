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

#include <stdio.h>
#include <stdarg.h>

#include "scriptix/system.h"
#include "scriptix/compiler.h"
#include "common/strbuf.h"

using namespace Scriptix;
using namespace Scriptix::Compiler;

#define _test(expr) {if(!(expr)) return false;}

static
void
_sxp_node_error (CompilerNode* node, const char *msg, ...)
{
	char buffer[512];
	va_list va;

	va_start (va, msg);
	vsnprintf (buffer, sizeof(buffer), msg, va);
	va_end (va);

	ScriptManager.handle_error (node->file ? node->file : S("<input>"), node->line, String(buffer));
}

static
int
_sxp_count (CompilerNode *node) {
	int i = 0;
	while (node != NULL) {
		++ i;
		node = node->next;
	}
	return i;
}

bool
Scriptix::Compiler::Compiler::CompileNode (CompilerFunction* func, CompilerNode *node) {
	// for the jumps necessary
	unsigned long pos;
	unsigned long pos2;

	while (node != NULL) {
		// update debug info
		if (node->line != last_line) {
			debug.push_back(DebugMetaData(func->func->count - last_op, last_line));
			last_op = func->func->count;
			last_line = node->line;
		}

		// select operations...
		switch (node->type) {
			// no operation
			case SXP_NOOP:
				// ignore
				break;
			// basic mathematical operation (+, /, -, *)
			case SXP_MATH:
				_test(CompileNode (func, node->parts.nodes[0]))
				_test(CompileNode (func, node->parts.nodes[1]))
				func->func->add_opcode((sx_op_type)node->parts.op);
				break;
			// push data onto stack
			case SXP_DATA:
				func->func->add_value(node->parts.value);
				break;
			// unary negation
			case SXP_NEGATE:
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_opcode(OP_NEGATE);
				break;
			// unary not operation
			case SXP_NOT:
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_opcode(OP_NOT);
				break;
			// short-cut or operator
			case SXP_OR:
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_opcode(OP_TEST);
				func->func->add_opcode(OP_TJUMP);
				pos = func->func->count;
				func->func->add_oparg(0);
				func->func->add_opcode(OP_POP);
				_test(CompileNode (func, node->parts.nodes[1]))
				func->func->nodes[pos] = func->func->count - pos;
				break;
			// short cut and operation
			case SXP_AND:
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_opcode(OP_TEST);
				func->func->add_opcode(OP_FJUMP);
				pos = func->func->count;
				func->func->add_oparg(0);
				func->func->add_opcode(OP_POP);
				_test(CompileNode (func, node->parts.nodes[1]))
				func->func->nodes[pos] = func->func->count - pos;
				break;
			// call a function
			case SXP_INVOKE:
				_test(CompileNode (func, node->parts.nodes[1]))
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_opcode(OP_INVOKE);
				func->func->add_oparg (_sxp_count (node->parts.nodes[1]));
				break;
			// retrieve a variable/global
			case SXP_LOOKUP:
			{
				Value lfunc;
				Value gval;
				char found = 0;
				long index;

				// do variable lookup
				index = get_var(func, node->parts.name);
				if (index >= 0) {
					func->func->add_opcode(OP_LOOKUP);
					func->func->add_oparg(index);
					break;
				}

				// search for function
				for (FunctionList::iterator dfunc = funcs.begin(); dfunc != funcs.end(); ++dfunc) {
					if ((*dfunc)->name == node->parts.name) {
						func->func->add_value((IValue*)((*dfunc)->func));
						found = 1;
						break;
					}
				}
				// found, done, ok
				if (found) {
					break;
				}

				// external function?
				lfunc = ScriptManager.get_function(node->parts.name);
				if (lfunc) {
					// add function
					func->func->add_value(lfunc);
					break;
				}

				// global variable?
				index = get_global(node->parts.name);
				if (index >= 0) {
					func->func->add_value(globals);
					func->func->add_value(Value(index));
					func->func->add_opcode(OP_INDEX);
					break;
				}

				// global constant?
				gval = ScriptManager.get_global(node->parts.name);
				if (gval) {
					func->func->add_value(gval);
					break;
				}

				// failure...
				_sxp_node_error (node, "Error: Unknown identifier '%s'", node->parts.name.name().c_str());
				return false;
			}
			// declare a variable
			case SXP_DECLARE:
			{
				long index;

				// already exists?
				index = get_var(func, node->parts.name);
				if (index >= 0) {
					_sxp_node_error (node, "Error: Local identifier '%s' already declared", node->parts.name.name().c_str());
					return false;
				}

				// create
				index = add_var(func, node->parts.name);

				// assign
				if (node->parts.nodes[0] != NULL)
					_test(CompileNode (func, node->parts.nodes[0]))
				else
					func->func->add_value(Nil);
				func->func->add_opcode(OP_ASSIGN);
				func->func->add_oparg(index);
				break;
			}
			// set a variable
			case SXP_ASSIGN:
			{
				long index = get_var(func, node->parts.name);
				if (index < 0) {
					// global variable?
					index = get_global(node->parts.name);
					if (index >= 0) {
						// do a lookup
						func->func->add_value((IValue*)globals);
						func->func->add_value(Value(index));
						_test(CompileNode (func, node->parts.nodes[0]))
						func->func->add_opcode(OP_SETINDEX);
						break;
						
					} else {
						_sxp_node_error (node, "Error: Unkown identifier '%s'", node->parts.name.name().c_str());
						return false;
					}
				}
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_opcode(OP_ASSIGN);
				func->func->add_oparg(index);
				break;
			}
			// pop the return of the expression - single statement
			case SXP_STATEMENT:
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_opcode(OP_POP);
				break;
			// conditional block
			case SXP_IF:
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_opcode(OP_TEST);
				func->func->add_opcode(OP_POP);
				func->func->add_opcode(OP_FJUMP);
				pos = func->func->count;
				func->func->add_oparg(0);
				_test(CompileNode (func, node->parts.nodes[1]))
				if (node->parts.nodes[2]) { // else
					func->func->add_opcode(OP_JUMP);
					pos2 = func->func->count;
					func->func->add_oparg(0);
					func->func->nodes[pos] = func->func->count - pos;
					_test(CompileNode (func, node->parts.nodes[2]))
					func->func->nodes[pos2] = func->func->count - pos2;
				} else {
					func->func->nodes[pos] = func->func->count - pos;
				}
				break;
			// a kind of loop
			case SXP_LOOP:
				push_block(func->func);
				switch (node->parts.op) {
					case SXP_LOOP_WHILE:
						// while... do - test true, loop
						_test(CompileNode (func, node->parts.nodes[0]))
						func->func->add_opcode(OP_TEST);
						func->func->add_opcode(OP_POP);
						_test(add_breakOnFalse())
						_test(CompileNode (func, node->parts.nodes[1]))
						_test(add_continue())
						break;
					case SXP_LOOP_UNTIL:
						// until... do - test false, loop
						_test(CompileNode (func, node->parts.nodes[0]))
						func->func->add_opcode(OP_TEST);
						func->func->add_opcode(OP_POP);
						_test(add_breakOnTrue())
						_test(CompileNode (func, node->parts.nodes[1]))
						_test(add_continue())
						break;
					case SXP_LOOP_DOWHILE:
						// do... while - loop, test true
						_test(CompileNode (func, node->parts.nodes[1]))
						_test(CompileNode (func, node->parts.nodes[0]))
						func->func->add_opcode(OP_TEST);
						func->func->add_opcode(OP_POP);
						func->func->add_opcode(OP_TJUMP);
						func->func->add_oparg(BlockStart() - func->func->count);
						break;
					case SXP_LOOP_DOUNTIL:
						// do... until - loop, test false
						_test(CompileNode (func, node->parts.nodes[1]))
						_test(CompileNode (func, node->parts.nodes[0]))
						func->func->add_opcode(OP_TEST);
						func->func->add_opcode(OP_POP);
						func->func->add_opcode(OP_FJUMP);
						func->func->add_oparg(BlockStart() - func->func->count);
						break;
					case SXP_LOOP_FOREVER:
						// permanent loop - loop
						_test(CompileNode (func, node->parts.nodes[1]))
						_test(add_continue())
						break;
				}
				pop_block();
				break;
			// set a value in an array
			case SXP_SETINDEX:
				_test(CompileNode (func, node->parts.nodes[0]))
				_test(CompileNode (func, node->parts.nodes[1]))
				_test(CompileNode (func, node->parts.nodes[2]))
				func->func->add_opcode(OP_SETINDEX);
				break;
			// get a value from an array
			case SXP_GETINDEX:
				_test(CompileNode (func, node->parts.nodes[0]))
				_test(CompileNode (func, node->parts.nodes[1]))
				func->func->add_opcode(OP_INDEX);
				break;
			// generate an array from stack value
			case SXP_ARRAY:
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_opcode(OP_NEWARRAY);
				func->func->add_oparg(_sxp_count(node->parts.nodes[0]));
				break;
			// return from the current call stack
			case SXP_RETURN:
				if (node->parts.nodes[0] != NULL)
					_test(CompileNode (func, node->parts.nodes[0]))
				else
					func->func->add_value(Nil);
				func->func->add_opcode(OP_JUMP);
				returns.push_back(func->func->count);
				func->func->add_oparg(0);
				break;
			// break from current loop/block
			case SXP_BREAK:
				_test(add_break())
				break;
			// invoke a method on an object
			case SXP_METHOD:
				// value
				_test(CompileNode (func, node->parts.nodes[0]))
				// args
				if (node->parts.nodes[1])
					_test(CompileNode (func, node->parts.nodes[1]))
				// call
				func->func->add_opcode(OP_METHOD);
				func->func->add_oparg (node->parts.name.value());
				func->func->add_oparg (_sxp_count (node->parts.nodes[1]));
				break;
			// convert a value into another type if possible
			case SXP_CAST:
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_value(new TypeValue(node->parts.type));
				func->func->add_opcode(OP_TYPECAST);
				break;
			// special loop with a setup section, test, post-body expression
			case SXP_FOR:
				// setup
				_test(CompileNode (func, node->parts.nodes[0]))
				// skip first increment
				func->func->add_opcode(OP_JUMP);
				pos = func->func->count;
				func->func->add_oparg(0);
				// begin loop
				_test(push_block(func->func));
				// increment
				_test(CompileNode (func, node->parts.nodes[2]))
				func->func->nodes[pos] = func->func->count - pos;
				// loop test
				_test(CompileNode (func, node->parts.nodes[1]))
				func->func->add_opcode(OP_TEST);
				func->func->add_opcode(OP_POP);
				_test(add_breakOnFalse())
				// body
				_test(CompileNode (func, node->parts.nodes[3]))
				// loop
				_test(add_continue())
				// end
				pop_block();
				break;
			// return to start of current loop/block
			case SXP_CONTINUE:
				_test(add_continue())
				break;
			// break current thread
			case SXP_YIELD:
				func->func->add_opcode(OP_YIELD);
				break;
			// check if a value is in an array
			case SXP_IN:
				// first put in list to check
				if (node->parts.nodes[1]) {
					_test(CompileNode (func, node->parts.nodes[1]))
				} else {
					func->func->add_value(Nil);
				}
				// then put in item to check for
				if (node->parts.nodes[0]) {
					_test(CompileNode (func, node->parts.nodes[0]))
				} else {
					func->func->add_value(Nil);
				}
				// set op
				func->func->add_opcode(OP_IN);
				break;
			// set a Struct property
			case SXP_SETPROPERTY:
				// object
				_test(CompileNode (func, node->parts.nodes[0]))
				// value
				_test(CompileNode (func, node->parts.nodes[1]))
				func->func->add_opcode(OP_SET_PROPERTY);
				func->func->add_oparg(node->parts.name.value());
				break;
			// retrieve a Struct property
			case SXP_GETPROPERTY:
				// object
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_opcode(OP_GET_PROPERTY);
				func->func->add_oparg(node->parts.name.value());
				break;
			// iterate over the items in a specialized list
			case SXP_FOREACH:
			{
				// set variable
				long index = get_var(func, node->parts.name);
				if (index < 0) {
					index = add_var(func, node->parts.name);
				}

				// set iterator object
				_test(CompileNode (func, node->parts.nodes[0]))
				// set start
				push_block(func->func);
				// iterator call
				func->func->add_opcode(OP_ITER);
				func->func->add_oparg(index);
				// jump on end
				_test(add_breakOnFalse())
				// body
				_test(CompileNode (func, node->parts.nodes[1]))
				// jump to begin
				_test(add_continue())
				// end
				pop_block();
				func->func->add_opcode(OP_POP);
				break;
			}
			// force a value into a string if possible
			case SXP_STRINGCAST:
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_opcode(OP_STRINGCAST);
				break;
			// force a value into an int if possible
			case SXP_INTCAST:
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_opcode(OP_INTCAST);
				break;
			// copy a value
			case SXP_COPY:
				func->func->add_oparg(node->parts.op);
				func->func->add_opcode(OP_COPY);
				break;
			// concatenate two strings
			case SXP_CONCAT:
				_test(CompileNode (func, node->parts.nodes[0]))
				_test(CompileNode (func, node->parts.nodes[1]))
				func->func->add_opcode(OP_CONCAT);
				break;
			// create a new stream
			case SXP_STREAM:
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_opcode(OP_STREAM_NEW);
				_test(CompileNode (func, node->parts.nodes[1]))
				func->func->add_opcode(OP_STREAM_END);
				break;
			// stream an item
			case SXP_STREAM_ITEM:
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_opcode(OP_STREAM_ITEM);
				break;
			// do a dream op
			case SXP_STREAM_OP:
				_test(CompileNode (func, node->parts.nodes[1]))
				_test(CompileNode (func, node->parts.nodes[0]))
				func->func->add_opcode(OP_INVOKE);
				func->func->add_oparg (_sxp_count (node->parts.nodes[1]) + 1);
				break;
			// Internal error
			default:
				Error(S("Internal error: missing opcode support"));
				break;
		}
		node = node->next;
	}

	return true;
}

int
Scriptix::Compiler::Compiler::Compile(void) {
	// make function data
	for (FunctionList::iterator func = funcs.begin(); func != funcs.end(); ++func) {
		(*func)->func = new Function ((*func)->name, ((*func)->vars).size());
		if (!(*func)->func) {
			Error(S("Failed to create function"));
			return -1;
		}
		(*func)->func->file = file;
	}

	// create type extensions
	for (ExtendList::iterator extend = extends.begin(); extend != extends.end(); ++extend) {
		for (CompilerExtend::MethodList::iterator func = (*extend)->methods.begin(); func != (*extend)->methods.end(); ++func) {
			if ((*extend)->type->get_method((*func)->name) != NULL) {
				StringBuffer errmsg;
				errmsg << "Attempt to extend type '";
				errmsg << (*extend)->type->get_name().name().c_str();
				errmsg << "' with method '";
				errmsg << (*func)->name.name().c_str();
				errmsg << "' which already exists";
				Error(errmsg.str());
				return -1;
			}

			// create function
			(*func)->func = new Function ((*func)->name, ((*func)->vars).size());
			if (!(*func)->func) {
				Error(S("Failed to create function"));
				return -1;
			}
			(*func)->func->file = file;
		}
	}

	// compile blocks
	for (FunctionList::iterator func = funcs.begin(); func != funcs.end(); ++func) {
		// optimize
		(*func)->body = sxp_transform ((*func)->body);

		// reset file/line
		debug.clear();
		last_line = line;
		last_op = 0;

		// compile node
		if (!CompileNode (*func, (*func)->body))
			return -1; // failed
		(*func)->func->add_value(Nil);
		debug.push_back(DebugMetaData((*func)->func->count - last_op, last_line));
		(*func)->func->debug = (DebugMetaData*)GC_MALLOC(sizeof(DebugMetaData) * debug.size());
		memcpy((*func)->func->debug, &debug[0], sizeof(DebugMetaData) * debug.size());

		// return calls
		while (!returns.empty()) {
			(*func)->func->nodes[returns.front()] = (long)(*func)->func->count - returns.front();
			returns.erase(returns.begin());
		}

		// variable count
		(*func)->func->varc = (*func)->vars.size();

		// now check if function is valid with the user handler
		if (handler && !handler->handle_function((*func)->func, (*func)->pub)) {
			Error(String(S("Rejected function ")) + String((*func)->name.name().c_str()));
			return -1;
		}
	}

	// compile type method extensions
	for (ExtendList::iterator extend = extends.begin(); extend != extends.end(); ++extend) {
		for (CompilerExtend::MethodList::iterator func = (*extend)->methods.begin(); func != (*extend)->methods.end(); ++func) {
			// optimize
			(*func)->body = sxp_transform ((*func)->body);

			// reset file/line
			debug.clear();
			last_line = line;
			last_op = 0;

			// compile
			if (!CompileNode (*func, (*func)->body))
				return -1; // failed
			(*func)->func->add_value(Nil);
			debug.push_back(DebugMetaData((*func)->func->count - last_op, last_line));
			(*func)->func->debug = (DebugMetaData*)GC_MALLOC(sizeof(DebugMetaData) * debug.size());
			memcpy((*func)->func->debug, &debug[0], sizeof(DebugMetaData) * debug.size());

			// return calls
			while (!returns.empty()) {
				(*func)->func->nodes[returns.front()] = (long)(*func)->func->count - returns.front();
				returns.erase(returns.begin());
			}

			// variable count
			(*func)->func->varc = (*func)->vars.size();
		}
	}

	// everything went right, extend types
	for (ExtendList::iterator extend = extends.begin(); extend != extends.end(); ++extend) {
		for (CompilerExtend::MethodList::iterator func = (*extend)->methods.begin(); func != (*extend)->methods.end(); ++func) {
			(*extend)->type->add_method((*func)->func->id, (*func)->func);
		}
	}

	// everything went right, publicize
	for (FunctionList::iterator func = funcs.begin(); func != funcs.end(); ++func) {
		// make public if public
		if ((*func)->pub)
			ScriptManager.add_function((*func)->func);
	}

	return 0;
}
