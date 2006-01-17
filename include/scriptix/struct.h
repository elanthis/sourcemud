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

#ifndef SCRIPTIX_STRUCT_H
#define SCRIPTIX_STRUCT_H

#include "common/gcmap.h"
#include "scriptix/atom.h"

namespace Scriptix {

class Struct : public IValue {

	// Constructor
	public:
	Struct ();

	virtual const TypeInfo* get_type () const;

	void set_property (Atom id, Value value);

	Value get_property (Atom id) const;

	protected:
	virtual void set_undefined_property (Atom id, Value value);
	virtual Value get_undefined_property (Atom id) const;

	// property data
	protected:
	typedef GCType::map<Atom, Value> datalist;
	datalist data;
};

class ScriptClass : public Struct {
	public:
	inline ScriptClass (const TypeInfo* s_type) : Struct(), type(s_type) {}

	virtual const TypeInfo* get_type () const { return type; }

	private:
	const TypeInfo* type;
};

} // namespace Scriptix

#endif
