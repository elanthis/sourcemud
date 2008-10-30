/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/creature.h"
#include "mud/command.h"
#include "mud/object.h"

/* BEGIN COMMAND
 *
 * name: kick
 * usage: kick <object>
 *
 * format: kick :0*
 *
 * END COMMAND */

void command_kick (Creature* ch, String argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_ANY, true);
	if (obj) {
		ch->do_kick (obj);
		return;
	}
	Portal* portal = ch->cl_find_portal (argv[0]);
	if (portal) {
		ch->do_kick (portal);
		return;
	}
}
