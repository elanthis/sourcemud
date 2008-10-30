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
 * name: give
 * usage: give <coins> [to] <recipient>
 *
 * format: give :0% to? :1*
 *
 * END COMMAND */

void command_give (Creature* ch, String argv[])
{
	static const char* usage = "You must supply a positive number of coins to give.\n";

	// get coin count
	if (!str_is_number(argv[0])) {
		*ch << usage;
		return;
	}
	int amount = tolong(argv[0]);
	if (amount <= 0) {
		*ch << usage;
		return;
	}

	// get target
	Creature* target = ch->cl_find_creature(argv[1]);
	if (!target)
		return;

	// do give
	ch->do_give_coins (target, amount);
}
