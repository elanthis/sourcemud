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

#include <iostream>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "scriptix/system.h"

using namespace Scriptix;

namespace Scriptix {
	SScriptManager ScriptManager;
}

namespace Scriptix {
	extern const TypeDef ScriptString_Type;
	extern const TypeDef Number_Type;
	extern const TypeDef Iterator_Type;
	extern const TypeDef Function_Type;
	extern const TypeDef Array_Type;
	extern const TypeDef TypeValue_Type;
	extern const TypeDef Struct_Type;
	extern const TypeDef ScriptClass_Type;
}

int
SScriptManager::initialize ()
{
	t_value = add_type(&IValue_Type);
	t_string = add_type(&ScriptString_Type);
	t_number = add_type(&Number_Type);
	t_function = add_type(&Function_Type);
	t_iterator = add_type(&Iterator_Type);
	t_array = add_type(&Array_Type);
	t_type = add_type(&TypeValue_Type);
	t_struct = add_type(&Scriptix::Struct_Type);
	t_script_class = add_type(&Scriptix::ScriptClass_Type);

	return 0;
}

void
SScriptManager::shutdown ()
{
}

void
SScriptManager::handle_error (String file, size_t line, String msg) {
	std::cerr << "Scriptix: " << file << ':' << line << ": " << msg << std::endl;
}
