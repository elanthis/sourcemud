/*
 * ZMP Example Implementation Library
 * *Heavily* modified for Source MUD usage
 * http://www.sourcemud.org/zmp/
 */
 
/* Copyright (C) 2004  Sean Middleditch
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

#if !defined(ZMP_H)
#define ZMP_H 1

#include <vector>

#include "common/string.h"
#include "common/types.h"
#include "common/imanager.h"
#include "net/telnet.h"

/* --- DATA TYPES ---- */

#define TELOPT_ZMP 93

/* define a command */
typedef void(*ZMPFunction)(class TelnetHandler* telnet, size_t argc, String argv[]);
struct ZMPCommand {
	String name;	// name of command
	bool wild;	// is this a wildcard match?
	ZMPFunction function;	// function to invoke
};

// build a ZMP pack to send
class ZMPPack
{
	public:
	ZMPPack (String command);

	// add an argument
	ZMPPack& add (String arg);
	ZMPPack& add (long);
	ZMPPack& add (ulong);
	inline ZMPPack& add (int i) { return add((long)i); }
	inline ZMPPack& add (uint i) { return add((ulong)i); }

	// send the ZMP pack along!
	inline void
	send (TelnetHandler* telnet)
	{ telnet->send_zmp(args.size(), &args[0]); }

	private:
	StringList args;
};

/* ---- ZMP INVOCATIONS ---- */

class SZMPManager : public IManager
{
	public:
	SZMPManager ();
	~SZMPManager ();

	// initialize basic system
	virtual int initialize ();

	// shutdown system
	virtual void shutdown ();

	// find a command
	ZMPCommand* lookup (String name);

	// add a new command
	int add (String name, ZMPFunction func);

	// see if a specific command/package is supported
	bool match (String pattern);

	private:
	// the list of commands
	typedef std::vector<ZMPCommand> ZMPCommandList;
	ZMPCommandList commands;
};
extern SZMPManager ZMPManager;

#endif
