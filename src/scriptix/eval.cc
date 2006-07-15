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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#undef HAVE_ALLOCA

// AIX requires this to be the first thing in the file. 
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
	#pragma alloca // indentation intentional
#  else
#   ifndef alloca // predefined by HP cc +Olibcalls
char *alloca ();
#   endif
#  endif
# endif
#endif


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "scriptix/system.h"
#include "scriptix/struct.h"
#include "scriptix/number.h"
#include "scriptix/array.h"
#include "scriptix/vimpl.h"
#include "scriptix/stream.h"
#include "common/log.h"

using namespace Scriptix;

OpCode Scriptix::OpCodeDefs[] = {
	{ "PUSH", 1 },
	{ "ADD", 0 },
	{ "SUBTRACT", 0 },
	{ "MULTIPLY", 0 },
	{ "DIVIDE", 0 },
	{ "NEGATE", 0 },
	{ "INVOKE", 1 },
	{ "CONCAT", 0 },
	{ "GT", 0 },
	{ "LT", 0 },
	{ "GTE", 0 },
	{ "LTE", 0 },
	{ "EQUAL", 0 },
	{ "NEQUAL", 0 },
	{ "NOT", 0 },
	{ "LOOKUP", 1 },
	{ "ASSIGN", 1 },
	{ "INDEX", 0 },
	{ "NEWARRAY", 1 },
	{ "TYPECAST", 0 },
	{ "STRINGCAST", 0 },
	{ "INTCAST", 0 },
	{ "SETINDEX", 0 },
	{ "METHOD", 2 },
	{ "JUMP", 1 },
	{ "POP", 0 },
	{ "TEST", 0 },
	{ "TJUMP", 1 },
	{ "FJUMP", 1 },
	{ "YIELD", 0 },
	{ "IN", 0 },
	{ "SET_PROPERTY", 1 },
	{ "GET_PROPERTY", 1 },
	{ "ITER", 1 },
	{ "COPY", 1 },
	{ "STREAM_NEW", 1 },
	{ "STREAM_ITEM", 2 },
	{ "STREAM_END", 1 },
};

int
SScriptManager::push_frame (const Function* func, size_t argc, Value argv[]) {
	// check number of args
	if (argc < func->argc)
		return raise_error(SXE_BADARGS, "Invalid number of arguments (%u of %u) to %s at %s:%lu", argc, func->argc, func->id.name().c_str(), func->file.c_str(), func->get_line_of(0));

	// push new frame
	frames.resize(frames.size() + 1);
	Frame& frame = frames.back();

	frame.top = data_stack.size() - (argv == NULL ? argc : 0);
	frame.flags = 0;
	frame.argc = argc;
	frame.func = func;

	// define variables for non-cfuncs
	Value* arglist = argv ? argv : &data_stack[data_stack.size() - argc];

	// C function
	if (func->cfunc) {
		frame.items = (Value*)GC_MALLOC(argc * sizeof(Value));
		if (frame.items == NULL)
			return SXE_NOMEM;
		memcpy(frame.items, arglist, argc * sizeof(Value));

	// Scriptix function
	} else {
		// argc items array (ick)
		if (func->varc != 0) {
			frame.items = (Value*)GC_MALLOC(func->varc * sizeof(Value));
			if (frame.items == NULL)
				return SXE_NOMEM;
			memset (frame.items, 0, func->varc * sizeof(Value));
		}

		// fill func args with arg list
		if (func->argc > 0)
			memcpy(frame.items, arglist, sizeof(Value) * func->argc);
	}

	return SXE_OK;
}

void
SScriptManager::pop_frame () {
	Frame& frame = frames.back();

	// unwind data stack 
	data_stack.resize(frame.top);

	// pop the frame
	frames.pop_back();
}

int
SScriptManager::invoke (Value self, Atom method, size_t argc, Value argv[], Value* retval)
{
	if (self.is_nil()) {
		if (retval != NULL)
			*retval = Nil;
		return SXE_NILCALL;
	}

	Function* func = Value(self).get_type()->get_method(method);
	if (func == NULL) {
		if (retval != NULL)
			*retval = Nil;
		return SXE_UNDEFINED;
	}

	Value nargv[argc + 1];
	nargv[0] = self;
	memcpy(&nargv[1], argv, argc * sizeof(Value));

	return invoke(func, argc + 1, nargv, retval);
}

int
SScriptManager::invoke (const Function* function, size_t argc, Value argv[], Value* retval)
{
	// define types here to speed up loop
	unsigned long count;
	int op;
	int err;
	long index;
	Value ret;
	Value value;
	const TypeInfo* type;
	Function* method;
	Atom name;
	size_t frame_top;

	if ((err = push_frame(function, argc, argv)) != SXE_OK)
		return err;

	state = STATE_RUNNING;
	frame_top = frames.size();

	while (frames.size() >= frame_top) {
run_code:
		// break processing if no longer running
		if (state != STATE_RUNNING)
			break;

		// working on a C function
		if (get_frame().func->cfunc) {
			ret = get_frame().func->cfunc(get_frame().argc, get_frame().items);
			pop_frame();
			push_value(ret);
			continue;
		}

		// empty function body
		if (get_frame().func->count == 0) {
			pop_frame();
			push_value(Nil);
			continue;
		}

		// iterate over ops
		while (state == STATE_RUNNING && get_frame().op_ptr < get_frame().func->count) {
			op = get_oparg();

			// DEBUG
			/*
			Log::Info
				<< get_frame().func->file.c_str()
				<< ':'
				<< get_frame().func->get_line_of(get_frame().op_ptr)
				<< " ["
				<< op
				<< "]\t"
				<< OpCodeDefs[op].name
				<< "\tD:"
				<< data_stack.size()
				<< "\tF:"
				<< frames.size();
			*/

			switch(op) {
			/* ----- OPERATORS ----- */
				case OP_PUSH:
					push_value((IValue*)get_oparg());
					continue;
				case OP_ADD:
					ret = Value (Number::to_int (get_value(2)) + Number::to_int (get_value()));
					pop_value(2);
					push_value(ret);
					break;
				case OP_SUBTRACT:
					ret = Value (Number::to_int (get_value(2)) - Number::to_int (get_value()));
					pop_value(2);
					push_value(ret);
					break;
				case OP_MULTIPLY:
					ret = Value (Number::to_int (get_value(2)) * Number::to_int (get_value()));
					pop_value(2);
					push_value(ret);
					break;
				case OP_DIVIDE:
					// divide by 0 check
					if (Number::to_int(get_value()) == 0) {
						pop_value(2);
						raise_error(SXE_DIVZERO, "Division by zero");
					} else {
						ret = Value (Number::to_int (get_value(2)) / Number::to_int (get_value()));
						pop_value(2);
						push_value(ret);
					}
					break;
				case OP_NEGATE:
					ret = Value (- Number::to_int (get_value()));
					pop_value();
					push_value(ret);
					break;
				case OP_INVOKE:
					count = get_oparg();
					value = get_value();
					pop_value();

					if (!Value(value).is_function()) {
						pop_value(count);
						raise_error(SXE_BADTYPE, "Invoked data is not a function");
						break;
					}

					push_frame(((Function*)value.get()), count, NULL);
					goto run_code; // jump to executation stage
				case OP_CONCAT:
					value = Value(get_value(2)).to_string();
					if (value) {
						Value second = Value(get_value()).to_string();
						pop_value(2);
						if (second) {
							ret = Value(value.get_string() + second.get_string());
							push_value(ret);
						} else {
							push_value(Nil);
						}
					// not supported...
					} else {
						pop_value(2);
						push_value(Nil);
					}
					break;
				case OP_GT:
					ret = Value (Value(get_value(2)).compare( get_value()) > 0);
					pop_value(2);
					push_value(ret);
					break;
				case OP_LT:
					ret = Value (Value(get_value(2)).compare( get_value()) < 0);
					pop_value(2);
					push_value(ret);
					break;
				case OP_GTE:
					ret = Value (Value(get_value(2)).compare( get_value()) >= 0);
					pop_value(2);
					push_value(ret);
					break;
				case OP_LTE:
					ret = Value (Value(get_value(2)).compare( get_value()) <= 0);
					pop_value(2);
					push_value(ret);
					break;
				case OP_EQUAL:
					ret = Value (Value(get_value(2)).equal( get_value()));
					pop_value(2);
					push_value(ret);
					break;
				case OP_NEQUAL:
					ret = Value (!Value(get_value(2)).equal( get_value()));
					pop_value(2);
					push_value(ret);
					break;
				case OP_NOT:
					ret = Value (!Value(get_value()).is_true());
					pop_value();
					push_value(ret);
					break;
				case OP_LOOKUP:
					push_value(get_frame().items[get_oparg()]);
					break;
				case OP_ASSIGN:
					get_frame().items[get_oparg()] = get_value();
					break;
				case OP_INDEX:
					value = get_value(2);
					index = Number::to_int(Value(get_value()).to_int());
					if (Value(value).is_array()) {
						ret = ((Array*)value.get())->get_index(index);
						pop_value(2);
						push_value(ret);
					} else {
						pop_value(2);
						raise_error(SXE_BADTYPE, "Instance is not an array type in index operation");
					}
					break;
				case OP_NEWARRAY:
					count = get_oparg();
					if (count > 0) {
						ret = new Array(count, &data_stack[data_stack.size() - count]);
						pop_value(count);
					} else {
						ret = new Array;
					}
					push_value(ret);
					break;
				case OP_TYPECAST:
					// get type
					value = get_value();
					type = ((TypeValue*)value.get())->get_type_ptr();

					// get value
					value = get_value(2);
					if (Value(value).is_a(type)) {
						// pop type
						pop_value();
					} else {
						// push NULL/false value
						pop_value(2);
						push_value(Nil);
					}
					break;
				case OP_STRINGCAST:
					// get value
					value = get_value();
					pop_value();
					push_value(Value(value).to_string());
					break;
				case OP_INTCAST:
					// get value
					value = get_value();
					pop_value();
					push_value(Value(value).to_int());
					break;
				case OP_SETINDEX:
					value = get_value(3);
					index = Number::to_int(Value(get_value(2)).to_int());
					if (Value(value).is_array()) {
						ret = ((Array*)value.get())->set_index (index, get_value());
						pop_value(3);
						push_value(ret);
					} else {
						pop_value(3);
						raise_error(SXE_BADTYPE, "Instance is not an array type in set index operation");
					}
					break;
				case OP_METHOD:
					name = Atom::create(get_oparg());
					count = get_oparg() + 1;
					value = get_value(count); // the type

					if (!value) {
						pop_value(count);
						raise_error(SXE_NILCALL, "Value is nil for method call");
						break;
					}
					type = Value(value).get_type();
					if (type == NULL) {
						pop_value(count);
						raise_error(SXE_NILCALL, "Value has no type for method call");
						break;
					}
					method = type->get_method(name);
					if (method == NULL) {
						pop_value(count);
						type = Value(value).get_type();
						raise_error(SXE_UNDEFINED, "Method '%s' on type '%s' does not exist", name.name().c_str(), type->get_name().name().c_str());
						break;
					}

					push_frame(method, count, NULL);
					goto run_code; // jump to executation stage
				case OP_JUMP:
					op = get_oparg();
					get_frame().op_ptr += op - 1;
					break;
				case OP_POP:
					pop_value();
					break;
				case OP_TEST:
					if (Value(get_value()).is_true())
						get_frame().flags |= CFLAG_TTRUE;
					else
						get_frame().flags &= ~CFLAG_TTRUE;
					break;
				case OP_TJUMP:
					op = get_oparg();
					if (get_frame().flags & CFLAG_TTRUE)
						get_frame().op_ptr += op - 1;
					break;
				case OP_FJUMP:
					op = get_oparg();
					if ((get_frame().flags & CFLAG_TTRUE) == 0)
						get_frame().op_ptr += op - 1;
					break;
				case OP_YIELD:
					// break - switch
					return state;
					break;
				case OP_IN:
					value = get_value(2);
					if (Value(value).is_array()) {
						if (((Array*)value.get())->has(get_value())) {
							pop_value(2);
							push_value(Value(1));
						} else {
							pop_value(2);
							push_value(Nil);
						}
					} else {
						pop_value(2);
						raise_error(SXE_BADTYPE, "Instance is not an array type in has operation");
					}
					break;
				case OP_SET_PROPERTY:
					value = get_value(2);
					name = Atom::create(get_oparg());
					if (Value(value).is_struct()) {
						((Struct*)value.get())->set_property(name, get_value());
						pop_value();
					} else {
						pop_value(2);
						raise_error(SXE_BADTYPE, "Attempt to get property value on non-structure");
					}
					break;
				case OP_GET_PROPERTY:
					value = get_value();
					name = Atom::create(get_oparg());
					pop_value();
					if (Value(value).is_struct()) {
						ret = ((Struct*)value.get())->get_property(name);
						push_value(ret);
					} else {
						raise_error(SXE_BADTYPE, "Attempt to get property value on non-structure");
					}
					break;
				case OP_ITER:
					name = Atom::create(get_oparg());
					value = get_value();
					// have we a nil?
					if (value.is_nil()) {
						// remove true flag
						get_frame().flags &= ~CFLAG_TTRUE;
					// have we an iterator?
					} else if (value.is_iterator()) {
						// get next
						if (!((Iterator*)value.get())->next(value)) {
							// remove true flag
							get_frame().flags &= ~CFLAG_TTRUE;
						} else {
							// set variable
							get_frame().items[name.value()] = value;
							get_frame().flags |= CFLAG_TTRUE;
						}
					// have we an array?
					} else if (Value(value).is_array()) {
						// generate iterator
						value = Array::get_iter((Array*)value.get());
						if (!value) {
							// remove true flag
							get_frame().flags &= ~CFLAG_TTRUE;
						} else {
							pop_value();
							push_value(value);
						}

						// get next
						if (!((Iterator*)value.get())->next(value)) {
							// remove true flag
							get_frame().flags &= ~CFLAG_TTRUE;
						} else {
							// set variable
							get_frame().items[name.value()] = value;
							get_frame().flags |= CFLAG_TTRUE;
						}
					// bad type
					} else {
						pop_value(); // remove type
						raise_error(SXE_BADTYPE, "Value is not an iterator or an array");
					}
					break;
				case OP_COPY:
					push_value(get_value(get_oparg()));
					break;
				case OP_STREAM_NEW:
				{
					value = get_value();
					pop_value();
					class IStreamSink* stream = value.get_stream();
					if (stream) {
						push_value(new Scriptix::Stream(stream));
					} else {
						raise_error(SXE_BADTYPE, "Value is not a stream sink");
					}
					break;
				}
				case OP_STREAM_ITEM:
					value = get_value(2);
					((Scriptix::Stream*)value.get())->stream(get_value());
					pop_value();
					break;
				case OP_STREAM_END:
					value = get_value();
					((Scriptix::Stream*)value.get())->end();
					pop_value();
					break;
			}
		}

		// pop frame, push return value
		ret = get_value();
		pop_frame();
		push_value(ret);
	}

	// return value
	ret = get_value();
	pop_value();

	// pop any extra frames (if we broke early)
	while (frames.size() > frame_top)
		pop_frame();

	// set return value
	if (retval != NULL)
		*retval = ret;

	// error occured?
	err = SXE_OK;
	if (state == STATE_FAILED)
		err = Number::to_int(get_value());

	// pop value, return error code
	return err;
}
