/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/string.h"
#include "mud/creature.h"
#include "mud/room.h"
#include "mud/command.h"
#include "mud/object.h"
#include "mud/player.h"

/* BEGIN COMMAND
 *
 * name: get
 * usage: get <item>
 * usage: get <item> from [in|on|under] <container>
 *
 * format: get :0*
 * format: get :0* from :1*
 * format: get :0* from :2in :1*
 * format: get :0* from :2on :1*
 * format: get :0* from :2under :1*
 *
 * END COMMAND */
void command_get(Creature* ch, std::string argv[])
{
	if (!argv[2].empty()) { // in, on, etc.
		ObjectLocation type;
		if (argv[1] == "on")
			type = ObjectLocation::ON;
		else if (argv[1] == "in")
			type = ObjectLocation::IN;

		// get container
		Object* cobj = ch->clFindObject(argv[2], GOC_ANY);
		if (!cobj)
			return;

		// no type, pick best, from in or on
		if (type == 0) {
			if (cobj->hasLocation(ObjectLocation::IN))
				type = ObjectLocation::IN;
			else if (cobj->hasLocation(ObjectLocation::ON))
				type = ObjectLocation::ON;
		} else if (!cobj->hasLocation(type)) {
			type = ObjectLocation::NONE; // invalidate type
		}

		// no valid type?
		if (type == ObjectLocation::NONE) {
			*ch << "You can't do that with " << StreamName(*cobj, DEFINITE) << ".\n";
			return;
		}

		// get object from container
		Object* obj = ch->clFindObject(argv[0], cobj, type);
		if (obj)
			ch->doGet(obj, cobj, type);
		else
			*ch << "Can't find '" << argv[0] << "' " << argv[1] << " " << StreamName(*cobj, DEFINITE) << ".\n";
		// get from the room
	} else {
		// coins?
		if (!argv[0].empty()) {
			Room* room = ch->getRoom();
			if (room == NULL) {
				*ch << "You are not in a room.\n";
				return;
			}

			uint max = room->getCoins();
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
			ch->giveCoins(amount);
			room->takeCoins(amount);
			if (amount == 1) {
				*ch << "You pick up a coin.\n";
				*room << StreamIgnore(ch) << StreamName(ch, DEFINITE, true) << " picks up a coin.\n";
			} else {
				*ch << "You pick up " << amount << " coins.\n";
				*room << StreamIgnore(ch) << StreamName(ch, DEFINITE, true) << " picks up " << amount << " coins.\n";
			}
			// object
		} else {
			Object* obj = ch->clFindObject(argv[0], GOC_ROOM);
			if (obj)
				ch->doGet(obj, NULL, ObjectLocation::NONE);
		}
	}
}

/* BEGIN COMMAND
 *
 * name: drop
 * usage: drop <item>
 *
 * format: drop :0*
 *
 * END COMMAND */
void command_drop(Creature* ch, std::string argv[])
{
	// object?
	if (argv[0].empty()) {
		Object* obj = ch->clFindObject(argv[0], GOC_HELD);
		if (obj)
			ch->doDrop(obj);
		// coins
	} else {
		// must be numeric
		if (!strIsNumber(argv[1])) {
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
		if ((uint)amount > ch->getCoins()) {
			if (ch->getCoins() == 1)
				*ch << "You only have one coin.\n";
			else
				*ch << "You only have " << ch->getCoins() << " coins.\n";
			return;
		}
		// must be in a room
		Room* room = ch->getRoom();
		if (room == NULL) {
			*ch << "You are not in a room.\n";
			return;
		}
		// do transfer
		room->giveCoins(amount);
		ch->takeCoins(amount);
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

/* BEGIN COMMAND
 *
 * name: put
 * usage: put <item> in|on|under <container>
 *
 * format: put :0* :2in :1*
 * format: put :0* :2on :1*
 * format: put :0* :2under :1*
 *
 * END COMMAND */
void command_put(Creature* ch, std::string argv[])
{
	Object* obj = ch->clFindObject(argv[0], GOC_HELD);
	if (!obj)
		return;

	Object* cobj = ch->clFindObject(argv[2], GOC_ANY);
	if (!cobj)
		return;

	if (argv[1] == "on")
		ch->doPut(obj, cobj, ObjectLocation::ON);
	else if (argv[1] == "in")
		ch->doPut(obj, cobj, ObjectLocation::IN);
}

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
void command_wear(Creature* ch, std::string argv[])
{
	Object* obj = ch->clFindObject(argv[0], GOC_HELD);
	if (obj)
		ch->doWear(obj);
}

/* BEGIN COMMAND
 *
 * name: remove
 * usage: remove <object>
 *
 * format: remove :0*
 *
 * END COMMAND */
void command_remove(Creature* ch, std::string argv[])
{
	Object* obj = ch->clFindObject(argv[0], GOC_WORN);
	if (obj)
		ch->doRemove(obj);
}

/* BEGIN COMMAND
 *
 * name: inventory
 *
 * END COMMAND */
void command_inventory(Player* player, std::string[])
{
	player->displayInventory();
}

/* BEGIN COMMAND
 *
 * name: swap
 *
 * format: swap
 *
 * END COMMAND */
void command_swap(Creature* ch, std::string[])
{
	*ch << "You swap the contents of your hands.\n";
	ch->swapHands();
}

/* BEGIN COMMAND
 *
 * name: give
 * usage: give <coins> [to] <recipient>
 *
 * format: give :0% to? :1*
 *
 * END COMMAND */
void command_give(Creature* ch, std::string argv[])
{
	static const char* usage = "You must supply a positive number of coins to give.\n";

	// get coin count
	if (!strIsNumber(argv[0])) {
		*ch << usage;
		return;
	}
	int amount = tolong(argv[0]);
	if (amount <= 0) {
		*ch << usage;
		return;
	}

	// get target
	Creature* target = ch->clFindCreature(argv[1]);
	if (!target)
		return;

	// do give
	ch->doGiveCoins(target, amount);
}
