/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/player.h"
#include "mud/server.h"
#include "mud/macro.h"
#include "mud/command.h"
#include "common/streams.h"
#include "common/mail.h"
#include "mud/color.h"
#include "mud/telnet.h"
#include "mud/settings.h"

/* BEGIN COMMAND
 *
 * name: reply
 * usage: reply <message>
 *
 * format: reply :0*
 *
 * END COMMAND */

void command_reply (Player* player, String argv[])
{
	player->do_reply(argv[0]);
}

void
Player::do_reply (String what)
{
	if (!last_tell) {
		*this << "No one has sent you a tell yet.\n";
		return;
	}

	Player* who = PlayerManager.get(last_tell);
	if (who) {
		*who << "[" << StreamName(this) << "]: " CTALK << what << CNORMAL "\n";
		who->last_tell = get_id();
		*this << "Reply sent to " << StreamName(who) << ".\n";
	} else {
		*this << "Player '" << last_tell << "' is not logged in.\n";
	}
}
