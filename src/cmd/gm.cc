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
#include "mud/player.h"
#include "mud/color.h"
#include "mud/zone.h"
#include "net/telnet.h"

/* BEGIN COMMAND
 *
 * name: gm announce
 * usage: gm announce <message>
 *
 * format: gm announce :0* (80)
 *
 * access: GM
 *
 * END COMMAND */
void command_gm_announce(Player* gm, std::string argv[])
{
	MZone.announce(std::string(CADMIN "Announcement: " CNORMAL) + std::string(argv[0]));
}

/* BEGIN COMMAND
 *
 * name: gm boot
 * usage: gm boot <player>
 *
 * format: gm boot %:0 (80)
 *
 * access: GM
 *
 * END COMMAND */
void command_gm_boot(Player* gm, std::string argv[])
{
	Player *cn = MPlayer.get(argv[0]);
	if (cn == gm) {
		*gm << "You cannot boot yourself.\n";
	} else if (cn == NULL) {
		*gm << "Player '" << argv[0] << "' is not logged in.\n";
	} else if (cn->getConn() == NULL) {
		*gm << "Player '" << argv[0] << "' is not actively connected.\n";
	} else {
		if (cn->getConn()) {
			*gm << "Booting " << StreamName(cn) << "...\n";
			*cn << CADMIN "You are being booted by a GM." CNORMAL "\n";
			Log::Info << "User " << cn->getAccount()->getId() << " booted by " << gm->getAccount()->getId();
			cn->disconnect();
		} else {
			*gm << StreamName(cn, DEFINITE, true) << " is already disconnected.\n";
		}
	}
}

