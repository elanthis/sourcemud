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

#ifndef SCRIPTIX_STRING_H
#define SCRIPTIX_STRING_H

#include <string>

#include "common/awestr.h"
#include "scriptix/value.h"
#include "scriptix/type.h"

namespace Scriptix {

class ScriptString : public IValue {
	public:
	ScriptString (const String& src);

	virtual const TypeInfo* get_type () const;

	// Methods
	public:
	static Value method_length (size_t argc, Value argv[]);
	static Value method_upper (size_t argc, Value argv[]);
	static Value method_lower (size_t argc, Value argv[]);
	static Value method_split (size_t argc, Value argv[]);
	static Value method_substr (size_t argc, Value argv[]);
	static Value method_trim (size_t argc, Value argv[]);
	static Value method_ltrim (size_t argc, Value argv[]);
	static Value method_rtrim (size_t argc, Value argv[]);
	static Value method_to_int (size_t argc, Value argv[]);

	static Value SMethodConcat (size_t argc, Value argv[]);

	// Query data
	public:
	inline size_t size (void) const { return data.length(); }
	inline const char* c_str (void) const { return data.c_str(); }
	inline String str (void) const { return data; }

	// Operations
	protected:
	virtual bool equal (Value other) const;
	virtual int compare (Value other) const;
	virtual bool is_true () const;
	virtual Value get_index (Value index) const;
	virtual bool has (Value value) const;

	private:
	String data;
};

} // namespace Scriptix

#endif
