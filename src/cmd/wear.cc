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
 * name: wear
 * usage: wear <object>
 * usage: equip <object>
 *
 * format: wear :0*
 * format: equip :0*
 *
 * END COMMAND */

void command_wear (Creature* ch, String argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_HELD);
	if (obj)
		ch->do_wear (obj);
}
