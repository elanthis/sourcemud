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

#include "scriptix/type.h"
#include "scriptix/function.h"
#include "scriptix/system.h"
#include "scriptix/vimpl.h"

using namespace Scriptix;

TypeInfo::TypeInfo (Atom s_name, const TypeInfo* s_parent) : name(s_name), parent(s_parent)
{
}

TypeInfo*
SScriptManager::add_type (TypeInfo* type)
{
	return types[type->get_name()] = type;
}

const TypeInfo* 
SScriptManager::get_type (Atom id) const
{
	// search
	TypeList::const_iterator i = types.find(id);
	if (i != types.end())
		return i->second;

	// no have
	return NULL;
}

TypeInfo* 
SScriptManager::get_type (Atom id)
{
	// search
	TypeList::iterator i = types.find(id);
	if (i != types.end())
		return i->second;

	// no have
	return NULL;
}

Function*
TypeInfo::get_method (Atom id) const
{
	const TypeInfo* type = this;
	while (type != NULL) {
		// search
		MethodList::const_iterator i = type->methods.find(id);
		if (i != type->methods.end())
			return i->second;
		type = type->parent;
	}

	return NULL;
}

int
TypeInfo::add_method (Atom id, Function* method)
{
	if (method == NULL)
		return SXE_INVALID;
	
	methods[id] = method;

	return SXE_OK;
}

int
TypeInfo::add_method (Function* method)
{
	if (method == NULL)
		return SXE_INVALID;
	
	methods[method->id] = method;

	return SXE_OK;
}

TypeValue::TypeValue(TypeInfo* s_type) : IValue(), type(s_type) {}

const TypeInfo*
TypeValue::get_type () const
{
	return ScriptManager.get_type_value_type();
}
	
Value
TypeValue::method_name (size_t argc, Value argv[])
{
	TypeValue* self = (TypeValue*)argv[0].get();
	return String (self->type->get_name().name());
}

Value
TypeValue::method_add_method (size_t argc, Value argv[])
{
	TypeValue* self = (TypeValue*)argv[0].get();
	TypeInfo* type = self->get_type_ptr();

	if (!Value(argv[1]).is_string()) {
		ScriptManager.raise_error(SXE_BADARGS, "Argument 1 to Type::addMethod() is not a string");
		return Nil;
	}
	if (!Value(argv[2]).is_function()) {
		ScriptManager.raise_error(SXE_BADARGS, "Argument 2 to Type::addMethod() is not a function");
		return Nil;
	}

	Atom id = Atom(argv[1].get_string());

	Function* func = type->get_method(id);
	if (func != NULL) {
		ScriptManager.raise_error(SXE_EXISTS, "Method named %s already exists on type %s", id.name().c_str(), type->get_name().name().c_str());
	}

	type->add_method(id, (Function*)argv[2].get());

	return argv[2];
}

bool
TypeValue::equal (Value other)
{
	if (other.get_type() != TypeValue::get_type())
		return false;

	return ((TypeValue*)other.get())->type == type;
}
