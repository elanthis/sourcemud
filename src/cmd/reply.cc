/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
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
 * name: tell
 * usage: tell <player> <message>
 *
 * format: tell :0% :1*
 *
 * END COMMAND */

void command_tell (Player* player, String argv[])
{
	Player* cn = PlayerManager.get(argv[0]);
	if (cn) {
		player->do_tell(cn, argv[1]);
	} else {
		*player << "Player '" << argv[0] << "' is not logged in.\n";
	}
}

void
Player::do_tell (Player* who, String what)
{
	*who << "[" << StreamName(this) << "]: " CTALK << what << CNORMAL "\n";
	who->last_tell = get_id();
	*this << "Message sent to " << StreamName(who) << ".\n";
}
