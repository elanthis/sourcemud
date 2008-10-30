/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/creature.h"
#include "mud/command.h"
#include "mud/portal.h"

/* BEGIN COMMAND
 *
 * name: close
 * usage: close <portal>
 *
 * format: close :0*
 *
 * END COMMAND */

void command_close (Creature* ch, String argv[]) {
	Portal* portal;
	if ((portal = ch->cl_find_portal (argv[0])) != NULL) {
		if (portal->is_door ())
			ch->do_close (portal);
		else
			*ch << StreamName(portal, DEFINITE, true) << " is not a door.\n";
	}
}
