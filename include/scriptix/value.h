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

#ifndef SCRIPTIX_VALUE_H
#define SCRIPTIX_VALUE_H

#include "common/gcbase.h"
#include "common/string.h"

namespace Scriptix {

class TypeInfo;
class Value;

class IValue : public GCType::GC {
	// Constructor/destructor
	public:
	inline IValue () : gc() {}
	virtual ~IValue (void) {}

	// Operations
	virtual bool equal (Value other) const;
	virtual int compare (Value other) const;
	virtual bool is_true () const;
	virtual const TypeInfo* get_type (void) const = 0;
};

class Value : public GCType::GC {
	public:
	// create
	inline Value () : value(NULL) {}
	inline Value (IValue* s_value) : value(s_value) {}
	inline Value (const Value& s_value) : value(s_value.value) {}

	// convert
	Value (StringArg value);
	Value (long value);

	inline Value (const char* s_value) { *this = Value(String(s_value)); }
	inline Value (int s_value) { *this = Value((long)s_value); }

	// get IValue*
	inline operator IValue* () { return value; }
	inline operator const IValue* () const { return value; }

	inline IValue* get () { return value; }
	inline const IValue* get () const { return value; }

	// tests
	inline operator bool () const { return is_true(); }

	inline bool operator< (const Value& other) const { return compare(other) < 0; }
	inline bool operator<= (const Value& other) const { return compare(other) <= 0; }
	inline bool operator> (const Value& other) const { return compare(other) > 0; }
	inline bool operator>= (const Value& other) const { return compare(other) >= 0; }
	inline bool operator== (const Value& other) const { return equal(other); }
	inline bool operator!= (const Value& other) const { return !equal(other); }

	// script operators
	bool equal (Value other) const;
	int compare (Value other) const;
	bool is_true () const;
	inline bool is_false () const { return !is_true(); }
	Value to_string () const;
	Value to_int () const;

	// get value as native type
	String get_string () const;
	long get_int () const;
	bool get_bool () const;

	// type check
	const TypeInfo* get_type () const;
	bool is_a (const TypeInfo* type) const;

	inline bool is_int () const;
	inline bool is_string () const;
	inline bool is_array () const;
	inline bool is_type () const;
	inline bool is_function () const;
	inline bool is_struct () const;
	inline bool is_iterator () const;
	inline bool is_nil () const;
	
	private:
	IValue* value;
};

// Nil
class IValue* const Nil = NULL;

} // namespace Scriptix

#endif
