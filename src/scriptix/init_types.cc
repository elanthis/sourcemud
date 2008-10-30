/*
 * Scriptix - Lite-weight scripting interface
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

#include "scriptix/array.h"
#include "scriptix/system.h"
#include "scriptix/string.h"
#include "scriptix/number.h"
#include "scriptix/iterator.h"

int
Scriptix::SScriptManager::initialize ()
{
	t_nil = add_type(new TypeInfo(Atom(S("Nil")), NULL));
	t_value = add_type(new TypeInfo(Atom(S("Value")), NULL));

	t_array = add_type(new TypeInfo(Atom(S("Array")), t_value));
	t_function = add_type(new TypeInfo(Atom(S("Function")), t_value));
	t_iterator = add_type(new TypeInfo(Atom(S("Iterator")), t_value));
	t_number = add_type(new TypeInfo(Atom(S("Int")), t_value));
	t_stream = add_type(new TypeInfo(Atom(S("Stream")), t_value));
	t_string = add_type(new TypeInfo(Atom(S("String")), t_value));
	t_struct = add_type(new TypeInfo(Atom(S("Struct")), t_value));
	t_type = add_type(new TypeInfo(Atom(S("Type")), t_value));

	t_iterator->add_method(new Function(Atom(S("next")), 0, Iterator::method_next));

	t_type->add_method(new Function(Atom(S("name")), 0, TypeValue::method_name));
	t_type->add_method(new Function(Atom(S("addMethod")), 2, TypeValue::method_add_method));

	t_number->add_method(new Function(Atom(S("toString")), 0, Number::method_to_string));

	t_string->add_method(new Function(Atom(S("length")), 0, ScriptString::method_length));
	t_string->add_method(new Function(Atom(S("toInt")), 0, ScriptString::method_to_int));
	t_string->add_method(new Function(Atom(S("upper")), 0, ScriptString::method_upper));
	t_string->add_method(new Function(Atom(S("lower")), 0, ScriptString::method_lower));
	t_string->add_method(new Function(Atom(S("substr")), 2, ScriptString::method_substr));
	t_string->add_method(new Function(Atom(S("split")), 1, ScriptString::method_split));
	t_string->add_method(new Function(Atom(S("trim")), 0, ScriptString::method_trim));

	t_array->add_method(new Function(Atom(S("length")), 0, Array::method_length));
	t_array->add_method(new Function(Atom(S("append")), 1, Array::method_append));
	t_array->add_method(new Function(Atom(S("remove")), 1, Array::method_remove));
	t_array->add_method(new Function(Atom(S("iter")), 0, Array::method_length));

	return 0;
}
