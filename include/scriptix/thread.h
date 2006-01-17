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

#ifndef SCRIPTIX_THREAD_H
#define SCRIPTIX_THREAD_H

#include <vector>

namespace Scriptix {

// Call-stack flags
typedef enum {
	CFLAG_TTRUE = (1 << 0),
} sx_call_flags;

// Thread states
typedef enum {
	STATE_READY = 0,	// ready to run
	STATE_RUNNING,		// currently running
	STATE_FINISHED,		// execution complete
	STATE_FAILED,		// runtime error
	STATE_RETURN,		// in return
} sx_state_type;

class Frame {
	public:
	Frame(void) : func(NULL), items(NULL), op_ptr(0), top(0), argc(0), flags(0) {}

	inline String* get_file () const { return func->file; }
	inline size_t get_line () const { return func->get_line_of(op_ptr); }
	
	private:
	const Function* func;
	Value* items;
	size_t op_ptr;
	size_t top;
	size_t argc;
	int flags;

	friend class SScriptManager;
};

} // namespace Scriptix

#endif
