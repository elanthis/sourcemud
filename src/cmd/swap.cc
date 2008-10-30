/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
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
