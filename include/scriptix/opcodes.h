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

#ifndef SCRIPTIX_OPCODES_H
#define SCRIPTIX_OPCODES_H

namespace Scriptix {

// Byte-code ops
typedef enum {
	OP_PUSH = 0,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_NEGATE,
	OP_INVOKE,
	OP_CONCAT,
	OP_GT,
	OP_LT,
	OP_GTE = 10,
	OP_LTE,
	OP_EQUAL,
	OP_NEQUAL,
	OP_NOT,
	OP_LOOKUP,
	OP_ASSIGN,
	OP_INDEX,
	OP_NEWARRAY,
	OP_TYPECAST,
	OP_STRINGCAST = 20,
	OP_INTCAST,
	OP_SETINDEX,
	OP_METHOD,
	OP_JUMP,
	OP_POP,
	OP_TEST,
	OP_TJUMP,
	OP_FJUMP,
	OP_YIELD,
	OP_IN = 30,
	OP_SET_PROPERTY,
	OP_GET_PROPERTY,
	OP_ITER,
	OP_COPY = 34,
} sx_op_type;

// Define opcodes
class OpCode {
	public:
	const char* name;
	unsigned char args;
};

extern OpCode OpCodeDefs[];

}; // namespace Scriptix

#endif
