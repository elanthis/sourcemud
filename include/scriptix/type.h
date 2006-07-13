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

#ifndef SCRIPTIX_TYPE_H
#define SCRIPTIX_TYPE_H

#include "common/gcbase.h"
#include "common/gcmap.h"
#include "scriptix/atom.h"
#include "scriptix/value.h"

namespace Scriptix {

// type def for callbacks
typedef class Value (*sx_cfunc)(size_t argc, class Value argv[]);

class MethodDef {
	public:
	String name;
	size_t argc;
	bool varg;
	void* method;
};

class TypeDef {
	public:
	String name;		///< Name of type.
	const TypeDef* parent;		///< Parent type.
	const MethodDef* methods;	///< Array of methods.
};

class TypeInfo : public GCType::GC {
	public:
	TypeInfo (const TypeDef* base, const TypeInfo* parent);

	TypeInfo (Atom name, const TypeInfo* parent);

	Atom get_name (void) const { return name; }
	const TypeInfo* get_parent (void) const { return parent; }
	class Function* get_method (Atom id) const;
	int add_method (Atom id, class Function* method);

	private:
	Atom name;			///< Name of type.
	const TypeInfo* parent;		///< Parent type.
	typedef GCType::map<Atom, class Function*> MethodList;
	MethodList methods;	///< List of methods.
};

class TypeValue : public IValue
{
	// Methods
	public:
	static Value method_name (size_t argc, Value argv[]);
	static Value method_add_method (size_t argc, Value argv[]);

	// Operators
	protected:
	virtual bool equal (Value other);

	public:
	TypeValue (TypeInfo* our_type);

	virtual const TypeInfo* get_type () const;

	inline TypeInfo* get_type_ptr (void) const { return type; } // silly name from conflict

	// Data
	private:
	TypeInfo* type;
};

// Root typedef
extern const TypeDef IValue_Type;

// Creating new types
#define SX_TYPEIMPL(CPPNAME, SXNAME, CPPPARENT) \
	extern const Scriptix::TypeDef CPPNAME ## _Type; \
	const Scriptix::TypeDef CPPNAME ## _Type = { \
		S(SXNAME) , \
		&CPPPARENT ## _Type, \
		CPPNAME ## _Methods, \
	}; 
#define SX_NOMETHODS(CPPNAME) namespace { Scriptix::MethodDef CPPNAME ## _Methods[] = { { String(), 0, 0, NULL } }; }
#define SX_BEGINMETHODS(CPPNAME) namespace { Scriptix::MethodDef CPPNAME ## _Methods[] = {
#define SX_ENDMETHODS { String(), 0, 0, NULL } }; }
#define SX_DEFMETHOD(CPPNAME, SXNAME, ARGC, VARARG) { S(SXNAME), ARGC, VARARG, (void*)CPPNAME }, 

} // namespace Scriptix

#endif
