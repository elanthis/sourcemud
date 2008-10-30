/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/creature.h"
#include "mud/room.h"
#include "mud/command.h"
#include "mud/object.h"

/* BEGIN COMMAND
 *
 * name: drop
 * usage: drop <item>
 *
 * format: drop :0*
 *
 * END COMMAND */

void command_drop (Creature* ch, String argv[])
{
	// object?
	if (argv[0]) {
		Object* obj = ch->cl_find_object (argv[0], GOC_HELD);
		if (obj)
			ch->do_drop (obj);
	// coins
	} else {
		// must be numeric
		if (!str_is_number(argv[1])) {
			*ch << "You must specify a number of coins to drop.\n";
			return;
		}
		// must be positive
		int amount = tolong(argv[1]);
		if (amount <= 0) {
			*ch << "You only drop a positive number of coins.\n";
			return;
		}
		// must have enough coins
		if ((uint)amount > ch->get_coins()) {
			if (ch->get_coins() == 1)
				*ch << "You only have one coin.\n";
			else
				*ch << "You only have " << ch->get_coins() << " coins.\n";
			return;
		}
		// must be in a room
		Room* room = ch->get_room();
		if (room == NULL) {
			*ch << "You are not in a room.\n";
			return;
		}
		// do transfer
		room->give_coins(amount);
		ch->take_coins(amount);
		// print it out
		if (amount == 1) {
			*ch << "You drop a coin.\n";
			*room << StreamIgnore(ch) << StreamName(ch, DEFINITE, true) << " drops a coin.\n";
		} else {
			*ch << "You drop " << amount << " coins.\n";
			*room << StreamIgnore(ch) << StreamName(ch, DEFINITE, true) << " drops " << amount << " coins.\n";
		}
	}
}
