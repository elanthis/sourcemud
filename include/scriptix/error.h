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

#ifndef SCRIPTIX_ERROR_H
#define SCRIPTIX_ERROR_H

namespace Scriptix {

// Error Codes
typedef enum {
	SXE_OK = 0,	// no error
	SXE_NOMEM,	// out of memory
	SXE_BADTYPE,	// invalid type for operation
	SXE_UNDEFINED,	// undefined/nonexistant
	SXE_NILCALL,	// method call on nil value
	SXE_BADOP,	// invalid operator - internal error
	SXE_BOUNDS,	// value out of bounds
	SXE_NOTREADY,	// not ready to handle request
	SXE_INVALID,	// generic invalid request
	SXE_DISABLED, 	// operation disabled
	SXE_BUSY,	// busy, cannot complete request
	SXE_INTERNAL,	// internal, unknown error
	SXE_BADARGS,	// bad set or arguments; count or type
	SXE_EXISTS,	// already exists (duplicate)
	SXE_DIVZERO,	// divide by zero
} sx_err_type;

// errors
const char *str_error (sx_err_type err);

} // namespace Scriptix

#endif
