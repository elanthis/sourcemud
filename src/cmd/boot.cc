/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/creature.h"
#include "common/error.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/command.h"
#include "mud/player.h"
#include "common/streams.h"
#include "mud/color.h"
#include "mud/zone.h"
#include "mud/telnet.h"

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

void command_gm_boot (Player* gm, String argv[])
{
	Player *cn = PlayerManager.get(argv[0]);
	if (cn == gm) {
		*gm << "You cannot boot yourself.\n";
	} else if (cn == NULL) {
		*gm << "Player '" << argv[0] << "' is not logged in.\n";
	} else if (cn->get_conn() == NULL) {
		*gm << "Player '" << argv[0] << "' is not actively connected.\n";
	} else {
		if (cn->get_conn()) {
			*gm << "Booting " << StreamName(cn) << "...\n";
			*cn << CADMIN "You are being booted by a GM." CNORMAL "\n";
			Log::Info << "User " << cn->get_account()->get_id() << " booted by " << gm->get_account()->get_id();
			cn->disconnect();
		} else {
			*gm << StreamName(cn, DEFINITE, true) << " is already disconnected.\n";
		}
	}
}

