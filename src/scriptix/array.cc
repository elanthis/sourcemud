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

#include <string.h>

#include "scriptix.h"
#include "system.h"

#include <algorithm>
#include <iostream>

using namespace std;
using namespace Scriptix;

namespace {
	const int ARRAY_CHUNK = 32;
}

Value
Array::method_length (size_t argc, Value argv[])
{
	Array* self = (Array*)argv[0].get();
	return Number::create(self->list.size());
}

Value
Array::method_append (size_t argc, Value argv[])
{
	Array* self = (Array*)argv[0].get();
	return self->append (argv[1]);
}

Value
Array::method_remove (size_t argc, Value argv[])
{
	Array* self = (Array*)argv[0].get();
	GCType::vector<Value>::iterator i;
	while ((i = std::find(self->list.begin(), self->list.end(), argv[1])) != self->list.end())
		self->list.erase(i);
	return Value();
}

Value
Array::method_iter (size_t argc, Value argv[])
{
	Array* self = (Array*)argv[0].get();
	return self->get_iter();
}

// Our methods
SX_BEGINMETHODS(Array)
	SX_DEFMETHOD(Array::method_length, "length", 0, 0)
	SX_DEFMETHOD(Array::method_append, "append", 1, 0)
	SX_DEFMETHOD(Array::method_remove, "remove", 1, 0)
	SX_DEFMETHOD(Array::method_iter, "iter", 0, 0)
SX_ENDMETHODS

// Define type parameters
namespace Scriptix {
	SX_TYPEIMPL(Array, "Array", IValue, SX_TYPECREATE(Array))
}

Array::Array () : IValue(), list() { }

Array::Array (size_t n_size, Value n_list[]) : IValue(), list(n_size)
{
	memcpy(&list[0], n_list, n_size*sizeof(Value));
};

const TypeInfo*
Array::get_type () const
{
	return ScriptManager.get_array_type();
}

bool
Array::is_true () const
{
	return !list.empty();
}

Value
Array::get_index (long index) const
{
	// no data items?
	if (list.empty())
		return Nil;

	// wrap negative index
	if (index < 0) {
		index += list.size();
		if (index < 0) {
			index = 0;
		}
	}

	// top index
	if ((size_t)index >= list.size())
		index = (long)list.size() - 1;
	
	// return value
	return list[index];
}

Value
Array::set_index (long index, Value value)
{
	// no data items?
	if (list.empty())
		return Nil;

	// wrap negative index
	if (index < 0) {
		index += list.size();
		if (index < 0) {
			index = 0;
		}
	}

	// top index
	if ((size_t)index >= list.size())
		index = (long)list.size() - 1;
	
	// return value
	return list[index] = value;
}

Value
Array::append (Value value)
{
	list.push_back(value);
	return value;
}

bool
Array::has (Value value) const
{
	return std::find(list.begin(), list.end(), value) != list.end();
}

Iterator*
Array::get_iter ()
{
	return new ArrayIterator(this);
}

bool
Array::ArrayIterator::next(Value& value)
{
	if (index >= array->get_count()) {
		return false;
	} else {
		value = array->get_index(index++);
		return true;
	}
}
