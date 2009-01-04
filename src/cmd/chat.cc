/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/error.h"
#include "common/streams.h"
#include "mud/creature.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/command.h"
#include "mud/body.h"
#include "mud/player.h"
#include "mud/macro.h"
#include "mud/object.h"

/* BEGIN COMMAND
 *
 * name: say
 * usage: say <text>
 *
 * format: say :0*
 *
 * END COMMAND */
void command_say(Creature* ch, std::string argv[])
{
	ch->do_say(argv[0]);
}

/* BEGIN COMMAND
 *
 * name: sing
 * usage: sing <verse>
 *
 * format: sing :0*
 *
 * END COMMAND */
void command_sing(Creature* ch, std::string argv[])
{
	ch->do_sing(argv[0]);
}

/* BEGIN COMMAND
 *
 * name: reply
 * usage: reply <message>
 *
 * format: reply :0*
 *
 * END COMMAND */
void command_reply(Player* player, std::string argv[])
{
	player->do_reply(argv[0]);
}

void Player::do_reply(const std::string& what)
{
	if (last_tell.empty()) {
		*this << "No one has sent you a tell yet.\n";
		return;
	}

	Player* who = MPlayer.get(last_tell);
	if (who) {
		*who << "[" << StreamName(this) << "]: " CTALK << what << CNORMAL "\n";
		who->last_tell = get_id();
		*this << "Reply sent to " << StreamName(who) << ".\n";
	} else {
		*this << "Player '" << last_tell << "' is not logged in.\n";
	}
}

/* BEGIN COMMAND
 *
 * name: tell
 * usage: tell <player> <message>
 *
 * format: tell :0% :1*
 *
 * END COMMAND */
void command_tell(Player* player, std::string argv[])
{
	Player* cn = MPlayer.get(argv[0]);
	if (cn) {
		player->do_tell(cn, argv[1]);
	} else {
		*player << "Player '" << argv[0] << "' is not logged in.\n";
	}
}

void Player::do_tell(Player* who, const std::string& what)
{
	*who << "[" << StreamName(this) << "]: " CTALK << what << CNORMAL "\n";
	who->last_tell = get_id();
	*this << "Message sent to " << StreamName(who) << ".\n";
}
