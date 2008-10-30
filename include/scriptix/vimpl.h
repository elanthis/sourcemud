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

#ifndef SCRIPTIX_VIMPL_H
#define SCRIPTIX_VIMPL_H

#include "scriptix/value.h"
#include "scriptix/type.h"
#include "scriptix/system.h"

namespace Scriptix {

bool Value::is_int () const { return ((intptr_t)value) & 0x01; }
bool Value::is_string () const { return is_a(ScriptManager.get_string_type()); }
bool Value::is_array () const { return is_a(ScriptManager.get_array_type()); }
bool Value::is_type () const { return is_a(ScriptManager.get_type_value_type()); }
bool Value::is_function () const { return is_a(ScriptManager.get_function_type()); }
bool Value::is_struct () const { return is_a(ScriptManager.get_struct_type()); }
bool Value::is_iterator () const { return is_a(ScriptManager.get_iterator_type()); }
bool Value::is_nil () const { return value == NULL; }

} // namespace Scriptix

#endif
