/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_CREATION_H
#define AWEMUD_MUD_CREATION_H 1

#include "mud/telnet.h"
#include "mud/account.h"
#include "mud/account.h"

class TelnetModeNewCharacter : public ITelnetMode
{
	public:
	TelnetModeNewCharacter (TelnetHandler* s_handler) : ITelnetMode (s_handler) {}

	static TelnetModeNewCharacter* create(TelnetHandler* s_handler, Account* s_account);

	virtual int initialize () = 0;
	virtual void prompt () = 0;
	virtual void process (char* line) = 0;
	virtual void shutdown () = 0;
	virtual void finish () = 0;
};

#endif
