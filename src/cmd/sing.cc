/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "mud/creature.h"
#include "mud/command.h"

/* BEGIN COMMAND
 *
 * name: sing
 * usage: sing <verse>
 *
 * format: sing :0*
 *
 * END COMMAND */

void command_sing (Creature* ch, String argv[])
{
	ch->do_sing(argv[0]);
}
