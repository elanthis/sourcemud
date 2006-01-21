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
#include <ctype.h>

#include "scriptix/sxstring.h"
#include "scriptix/system.h"
#include "scriptix/number.h"
#include "scriptix/array.h"
#include "scriptix/vimpl.h"

using namespace Scriptix;

SX_BEGINMETHODS(ScriptString)
	SX_DEFMETHOD(ScriptString::method_length, "length", 0, 0)
	SX_DEFMETHOD(ScriptString::method_to_int, "toInt", 0, 0)
	SX_DEFMETHOD(ScriptString::method_upper, "upper", 0, 0)
	SX_DEFMETHOD(ScriptString::method_lower, "lower", 0, 0)
	SX_DEFMETHOD(ScriptString::method_substr, "substr", 2, 0)
	SX_DEFMETHOD(ScriptString::method_split, "split", 1, 0)
	SX_DEFMETHOD(ScriptString::method_trim, "trim", 0, 0)
	SX_DEFMETHOD(ScriptString::method_ltrim, "ltrim", 0, 0)
	SX_DEFMETHOD(ScriptString::method_rtrim, "rtrim", 0, 0)
SX_ENDMETHODS

namespace Scriptix {
	SX_TYPEIMPL(ScriptString, "String", IValue, SX_TYPECREATENONE(ScriptString))
}

ScriptString::ScriptString (const String& src) : IValue(), data(src) {
}

const TypeInfo*
ScriptString::get_type () const {
	return ScriptManager.get_string_type();
}

bool
ScriptString::equal (Value other) const
{
	if (!Value(other).is_string())
		return false;

	return data == ((ScriptString*)other.get())->data;
}

int
ScriptString::compare (Value other) const
{
	if (!Value(other).is_string())
		return -1;

	if (size() < ((ScriptString*)other.get())->size()) {
		return -1;
	} else if (size() > ((ScriptString*)other.get())->size()) {
		return 1;
	} else if (size() == 0) {
		return 0;
	}
	return strcmp (c_str(), ((ScriptString*)other.get())->c_str());
}

bool
ScriptString::is_true () const
{
	return !data.empty();
}

bool
ScriptString::has (Value value) const
{
	const char *c;

	if (!value.is_string())
		return false;

	// blank test - always in
	if (!((ScriptString*)value.get())->size())
		return true;
	// longer - can't be in
	if (((ScriptString*)value.get())->size() > size())
		return false;

	// scan and check
	for (c = c_str(); *c != '\0'; ++ c) {
		if (!strncasecmp (c, ((ScriptString*)value.get())->c_str(), ((ScriptString*)value.get())->size()))
			return true;
	}

	return false;
}

Value 
ScriptString::get_index (Value vindex) const
{
	long index;

	if (!Value(vindex).is_int())
		return Nil;
	
	index = Number::to_int(vindex);

	if (data.empty()) {
		return Nil;
	}
	if (index < 0) {
		index += size();
		if (index < 0) {
			index = 0;
		}
	}
	if ((size_t)index >= size()) {
		index = size() - 1;
	}
	
	return Value(String(c_str() + index, 1));
}

Value
ScriptString::method_length (size_t argc, Value argv[])
{
	ScriptString* self = (ScriptString*)argv[0].get();
	return Value ((long)self->size());
}

Value
ScriptString::method_to_int(size_t argc, Value argv[])
{
	ScriptString* self = (ScriptString*)argv[0].get();
	return Value (atoi (self->c_str()));
}

Value
ScriptString::method_split (size_t argc, Value argv[])
{
	ScriptString* self = (ScriptString*)argv[0].get();
	const char *c, *needle, *haystack;
	size_t nlen;

	if (!Value(argv[1]).is_string()) {
		ScriptManager.raise_error(SXE_BADARGS, "Argument 1 to ScriptString::split() is not a string");
		return Nil;
	}

	haystack = self->c_str();
	needle = ((ScriptString*)argv[1].get())->c_str();
	nlen = strlen (needle);

	Array* array = new Array();
	if (array == NULL) {
		return Nil;
	}

	// no needle
	if (nlen == 0) {
		Array::append (array, self);
		return array;
	}

	// find substr
	for (c = haystack; *c != '\0'; ++ c) {
		if (!strncasecmp (c, needle, strlen (needle))) {
			Array::append (array, Value(String(haystack, c - haystack)));
			haystack = c + nlen;
		}
	}

	// append last
	if (*haystack != '\0') {
		Array::append (array, Value(String(haystack)));
	}

	return array;
}

Value
ScriptString::method_substr (size_t argc, Value argv[])
{
	ScriptString* self = (ScriptString*)argv[0].get();
	int start, len;

	start = Number::to_int(argv[1]);
	len = Number::to_int(argv[2]);

	// valid starting location?
	if (start < 0 || (size_t)start >= self->size()) {
		// FIXME: perhaps an error?
		return Nil;
	}

	// trim len
	if ((size_t)(start + len) > self->size()) {
		len = self->size() - start;
	}

	// return value
	return Value(String(self->c_str() + start, len));
}

Value
ScriptString::method_lower (size_t argc, Value argv[])
{
	ScriptString* self = (ScriptString*)argv[0].get();
	return strlower(self->str());
}

Value
ScriptString::method_upper (size_t argc, Value argv[])
{
	ScriptString* self = (ScriptString*)argv[0].get();
	return strupper(self->str());
}

Value
ScriptString::method_trim (size_t argc, Value argv[])
{
	ScriptString* self = (ScriptString*)argv[0].get();
	size_t left = self->str().find_first_not_of(" \t\n");
	if (left == BaseString::npos)
		return new ScriptString("");
	size_t right = self->str().find_last_not_of(" \t\n");
	if (right == BaseString::npos)
		return new ScriptString("");
	return new ScriptString(self->str().substr(left, right - left + 1));
}

Value
ScriptString::method_ltrim (size_t argc, Value argv[])
{
	ScriptString* self = (ScriptString*)argv[0].get();
	size_t left = self->str().find_first_not_of(" \t\n");
	if (left == BaseString::npos)
		return new ScriptString("");
	return new ScriptString(self->str().substr(left, BaseString::npos));
}

Value
ScriptString::method_rtrim (size_t argc, Value argv[])
{
	ScriptString* self = (ScriptString*)argv[0].get();
	size_t right = self->str().find_last_not_of(" \t\n");
	if (right == BaseString::npos)
		return new ScriptString("");
	return new ScriptString(self->str().substr(0, right + 1));
}
