/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/telnet.h"
#include "mud/player.h"
#include "mud/login.h"

int TelnetModeNewCharacter::initialize ()
{
	return 1;
}

void TelnetModeNewCharacter::prompt ()
{
}

void TelnetModeNewCharacter::process (char* line)
{
}

void TelnetModeNewCharacter::shutdown ()
{
	get_handler()->set_mode(new TelnetModeMainMenu(get_handler(), account));
}

int SPlayerManager::create (TelnetHandler* telnet, Account* account)
{
	ITelnetMode* mode = new TelnetModeNewCharacter(telnet, account);
	if (mode != NULL) {
		telnet->set_mode(new TelnetModeNewCharacter(telnet, account));
		return 0;
	} else {
		return 1;
	}
}
