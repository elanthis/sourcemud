/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "mud/creature.h"
#include "mud/room.h"
#include "mud/command.h"
#include "mud/object.h"

/* BEGIN COMMAND
 *
 * name: get
 * usage: get <item>
 * usage: get <item> from [in|on|under] <container>
 *
 * format: get :0*
 * format: get :0* from :2(in,on,under)? :1*
 *
 * END COMMAND */

void command_get (Creature* ch, String argv[]) {
	if (!argv[2].empty()) { // in, on, etc.
		ObjectLocation type;
		if (argv[1] == "on")
			type = ObjectLocation::ON;
		else if (argv[1] == "in")
			type = ObjectLocation::IN;

		// get container
		Object* cobj = ch->cl_find_object (argv[2], GOC_ANY);
		if (!cobj)
			return;

		// no type, pick best, from in or on
		if (type == 0) {
			if (cobj->has_location(ObjectLocation::IN))
				type = ObjectLocation::IN;
			else if (cobj->has_location(ObjectLocation::ON))
				type = ObjectLocation::ON;
		} else if (!cobj->has_location(type)) {
			type = ObjectLocation::NONE; // invalidate type
		}
			
		// no valid type?
		if (type == ObjectLocation::NONE) {
			*ch << "You can't do that with " << StreamName(*cobj, DEFINITE) << ".\n";
			return;
		}

		// get object from container
		Object* obj = ch->cl_find_object(argv[0], cobj, type);
		if (obj)
			ch->do_get (obj, cobj, type);
		else
			*ch << "Can't find '" << argv[0] << "' " << argv[1] << " " << StreamName(*cobj, DEFINITE) << ".\n";
	// get from the room
	} else {
		// coins?
		if (argv[0].empty()) {
			Room* room = ch->get_room();
			if (room == NULL) {
				*ch << "You are not in a room.\n";
				return;
			}

			uint max = room->get_coins();
			int amount = 0;

			// how many?
			if (argv[3].empty()) {
				amount = max;
			} else {
				amount = tolong(argv[3]);
				if (amount <= 0) {
					*ch << "You can only get a positive number of coins.\n";
					return;
				}
			}

			// enough coins?
			if (max == 0) {
				*ch << "There aren't any coins here.\n";
				return;
			} else if (max < (uint)amount) {
				if (max == 1)
					*ch << "There is only 1 coin here.\n";
				else
					*ch << "There are only " << max << " coins here.\n";
				return;
			}

			// do transfer
			ch->give_coins(amount);
			room->take_coins(amount);
			if (amount == 1) {
				*ch << "You pick up a coin.\n";
				*room << StreamIgnore(ch) << StreamName(ch, DEFINITE, true) << " picks up a coin.\n";
			} else {
				*ch << "You pick up " << amount << " coins.\n";
				*room << StreamIgnore(ch) << StreamName(ch, DEFINITE, true) << " picks up " << amount << " coins.\n";
			}
		// object
		} else {
			Object* obj = ch->cl_find_object (argv[0], GOC_ROOM);
			if (obj)
				ch->do_get (obj, NULL, ObjectLocation::NONE);
		}
	}
}
