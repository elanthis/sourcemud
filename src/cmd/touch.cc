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
 * name: touch
 * usage: touch <object>
 *
 * format: touch :0*
 *
 * END COMMAND */

void command_touch (Creature* ch, String argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_ANY);
	if (obj)
		ch->do_touch (obj);
}
