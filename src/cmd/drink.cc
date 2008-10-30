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
 * name: drink
 * usage: drink <object>
 *
 * format: drink :0*
 *
 * END COMMAND */

void command_drink (Creature* ch, String argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_ANY);
	if (obj)
		ch->do_drink (obj);
}
