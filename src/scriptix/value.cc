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

#include "scriptix/system.h"
#include "scriptix/string.h"
#include "scriptix/number.h"
#include "scriptix/vimpl.h"

#include <iostream>
#include <string.h>

using namespace Scriptix;

// Root typedef
namespace { const MethodDef Value_Methods[] = { { String(), 0, 0, } }; }
const TypeDef Scriptix::IValue_Type = { S("Value"), NULL, Value_Methods, NULL };

// CONSTRUCTORS
Value::Value (StringArg s_value)
{
	value = new ScriptString(s_value);
}

Value::Value (long s_value)
{
	value = Number::create(s_value);
}

// FETCH NATIVE FORMAT

String
Value::get_string () const
{
	Value v = to_string();
	if (v.is_nil())
		return String();
	return ((ScriptString*)v.get())->str();
}

// STATIC FUNCTIONS

const TypeInfo*
Value::get_type () const
{
	if (value == NULL)
		return NULL;

	if ((intptr_t)value & 0x01)
		return ScriptManager.get_number_type();

	return value->get_type();
}

bool
Value::is_a (const TypeInfo* type) const
{
	const TypeInfo* my_type = get_type();

	while (my_type != NULL) {
		if (my_type == type)
			return true;
		my_type = my_type->get_parent();
	}

	return false;
}

bool
Value::equal (Value other) const
{
	if (value == other)
		return true;

	if (value != NULL && !is_int())
		return value->equal(other);
	else
		return false;
}

int
Value::compare (Value other) const
{
	if (value == other)
		return 0;

	// handle nil values somewhat gracefully
	if (is_nil())
		return -1; // non-nil bigger than nil
	else if (other.is_nil())
		return 1; // non-nil bigger than nil

	// do compare
	if (value != NULL && !is_int())
		return value->compare(other);
	else if (is_int()) {
		if (Number::to_int(value) < Number::to_int(other))
			return -1;
		else
			return 1;
	} else
		return 1; // default
}

bool
Value::is_true () const
{
	if (value == NULL)
		return false;
	else if (is_int())
		return Number::to_int(value);
	else
		return value->is_true();
}

Value
Value::to_string() const
{
	if (is_string())
		return value;

	Value ret;
	static Atom to_string_id = Atom(S("toString"));
	if (ScriptManager.invoke(value, to_string_id, 0, NULL, &ret))
		return Nil;

	if (Value(ret).is_string())
		return ret;

	return Nil;
}

Value
Value::to_int() const
{
	if (Number::is_number(value))
		return value;

	Value ret;
	static Atom to_int_id = Atom(S("toInt"));
	if (ScriptManager.invoke(value, to_int_id, 0, NULL, &ret))
		return Nil;

	if (Value(ret).is_int())
		return ret;

	return Nil;
}

// DEFAULT IMPLEMENTATIONS
bool
IValue::equal (Value other) const
{
	return this == other;
}

int
IValue::compare (Value other) const
{
	return !(this == other);
}

bool
IValue::is_true () const
{
	return true;
}
