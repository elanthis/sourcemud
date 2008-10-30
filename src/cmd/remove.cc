/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "mud/creature.h"
#include "mud/command.h"
#include "mud/object.h"

/* BEGIN COMMAND
 *
 * name: remove
 * usage: remove <object>
 *
 * format: remove :0*
 *
 * END COMMAND */

void command_remove (Creature* ch, String argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_WORN);
	if (obj)
		ch->do_remove (obj);
}
