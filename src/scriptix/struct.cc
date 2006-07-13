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
#include "scriptix/struct.h"
#include "scriptix/vimpl.h"

using namespace Scriptix;

// constructors
Struct::Struct () : IValue(), data()
{}

const TypeInfo*
Struct::get_type () const {
	return ScriptManager.get_struct_type();
}

SX_NOMETHODS(Struct);

// type definition
namespace Scriptix {
SX_TYPEIMPL(Struct, "Struct", IValue);
}

// Default undefinedined property operations
void
Struct::set_undefined_property (Atom id, Value value)
{
	data[id] = value;
}

Value
Struct::get_undefined_property (Atom) const
{
	// no-op
	return Nil;
}

// Member operations
void
Struct::set_property (Atom id, Value value)
{
	// try to set value
	datalist::iterator i = data.find(id);
	if (i != data.end()) {
		i->second = value;
		return;
	}

	// undefinedined - set that way then
	set_undefined_property (id, value);
}

Value
Struct::get_property (Atom id) const
{
	// try to find property
	datalist::const_iterator i = data.find(id);
	if (i != data.end()) {
		return i->second;
	}

	// undefinedined - get that way then
	return get_undefined_property (id);
}
