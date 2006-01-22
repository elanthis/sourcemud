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

#ifndef SCRIPTIX_NAME_H
#define SCRIPTIX_NAME_H

#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common/string.h"

namespace Scriptix {

// atom, unique name ID
class Atom {
	public:
	inline Atom () : id(0) {}
	inline Atom (const Atom& atom) : id(atom.id) {}
	Atom (StringArg name);
	inline Atom (const char* name) { *this = Atom(String(name)); }

	inline static Atom create (intptr_t value) { Atom atom; atom.id = value; return atom; }

	inline Atom& operator= (const Atom& atom) { id = atom.id; return *this; }
	inline bool operator== (const Atom& atom) const { return id == atom.id; }
	inline bool operator< (const Atom& atom) const { return id < atom.id; }

	String name () const;
	inline intptr_t value () const { return id; }

	private:
	intptr_t id;
};

} // namespace Scriptix

#endif
