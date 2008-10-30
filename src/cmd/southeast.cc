/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/creature.h"
#include "mud/room.h"
#include "mud/command.h"

/* BEGIN COMMAND
 *
 * name: southeast
 *
 * format: southeast (12)
 * format: se (12)
 *
 * END COMMAND */

void command_southeast (Creature* ch, String argv[]) {
	// must be in a room
	if (!ch->get_room()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->get_room()->get_portal_by_dir (PortalDir::SOUTHEAST);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->do_go(portal);
}
