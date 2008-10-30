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
 * name: swap
 *
 * format: swap
 *
 * END COMMAND */

void command_swap (Creature* ch, String[])
{
	*ch << "You swap the contents of your hands.\n";
	ch->swap_hands ();
}
