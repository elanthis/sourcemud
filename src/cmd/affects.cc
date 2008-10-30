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
 * name: affects
 *
 * format: affects
 *
 * END COMMAND */

void command_affects (Creature* ch, String argv[])
{
	ch->display_affects(*ch);
}
