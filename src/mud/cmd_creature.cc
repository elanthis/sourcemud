/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/creature.h"
#include "common/error.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/command.h"
#include "mud/body.h"
#include "mud/player.h"
#include "mud/macro.h"
#include "common/streams.h"
#include "mud/object.h"

void
Creature::process_command (String line)
{
	CommandManager.call (this, line);
}

// Helper functions for the Creature::cl_find_* functions
namespace {
	// skip a word and following white space
	void skip_word (CString& ptr)
	{
		// skip the word
		while (*ptr != 0 && !isspace(*ptr))
			++ptr;

		// skip the whitespace
		while (*ptr != 0 && isspace(*ptr))
			++ptr;
	}

	// return true if the string matches 'test' followed by a space or NUL
	bool word_match (CString string, CString test)
	{
		// if there's no match, it's false
		if (strncasecmp(string, test, strlen(test)))
			return false;

		// byte following the match must be a space or NULL
		return isspace(string[strlen(test)]) || string[strlen(test)] == 0;
	}

	// return true if a string has no more creatures
	inline bool str_empty (CString string)
	{
		return string[0] == 0;
	}

	// return a numeric value of the next word in the input
	uint str_value (CString string)
	{
		// first, check for simple number
		char* end;
		int value = strtoul (string, &end, 10);
		if (end != NULL) {
			// just a number:
			if (*end == 0 || isspace(*end))
				return value;

			// find out if it's a number followed by st, nd, rd, or th
			if (value % 10 == 1 && value != 11 && word_match(end, "st"))
				return value;
			else if (value % 10 == 2 && value != 12 && word_match(end, "nd"))
				return value;
			else if (value % 10 == 3 && value != 13 && word_match(end, "rd"))
				return value;
			else if ((value % 10 > 3 || value % 10 == 0 || (value >= 10 && value <= 13)) && word_match(end, "th"))
				return value;
		}

		// next, look for "other" or "second"
		if (prefix_match("other", string) || prefix_match("second", string))
			return 2;

		// try third
		if (prefix_match("third", string))
			return 3;

		// nobodies going to go higher than that with words, and if they do, fuck 'em
		return 0;
	}
}

// parse a command line to get an object
Object* 
Creature::cl_find_object (String line, int type, bool silent)
{
	uint matches;
	Object* object;
	const char* text = line.c_str();

	// check for arg
	if (str_empty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// do we have a my keyword?
	if (word_match (text, "my")) {
		type &= ~GOC_FLOOR; // clear floor flag
		skip_word(text);
	} else if (word_match(text, "the")) {
		skip_word(text);
	}

	// get index
	uint index = str_value(text);
	if (index)
		skip_word(text);
	else
		index = 1;

	// check for arg
	if (str_empty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// store name
	String name (text);

	// search room
	if (type & GOC_FLOOR && get_room() != NULL) {
		object = get_room()->find_object (name, index, &matches);
		if (object)
			return object;
		if (matches >= index) {
			*this << "You do not see '" << name << "'.\n";
			return NULL;
		}
		index -= matches; // update count; subtract matches
	}

	// do a sub search for on containers in the room, like tables
	if (type & GOC_SUB && get_room() != NULL) {
		for (EList<Object>::iterator iter = get_room()->objects.begin(); iter != get_room()->objects.end(); ++iter) {
			// have an ON container - search inside
			if (*iter != NULL && (*iter)->has_location(ObjectLocation::ON)) {
				if ((object = (*iter)->find_object (name, index, ObjectLocation::ON, &matches))) {
					return object;
				}
				if (matches >= index) {
					*this << "You do not see '" << name << "'.\n";
					return NULL;
				}
				index -= matches; // update count; subtract matches
			}
		}
	}

	// FIXME: these aren't handling the matches/index stuff:

	// try held
	if (type & GOC_HELD)
		if ((object = find_held (name)) != NULL)
			return object;

	// try worn
	if (type & GOC_WORN)
		if ((object = find_worn (name)) != NULL)
			return object;

	// print error
	if (!silent) {
		if (type & GOC_ROOM)
			*this << "You do not see '" << name << "'.\n";
		else if ((type & GOC_HELD) && !(type & GOC_WORN))
			*this << "You are not holding '" << name << "'.\n";
		else if ((type & GOC_WORN) && !(type & GOC_HELD))
			*this << "You are not wearing '" << name << "'.\n";
		else if ((type & GOC_HELD) && (type & GOC_WORN))
			*this << "You don't have '" << name << "'.\n";
	}
	return NULL;
}

// find an object inside a particular container
Object*
Creature::cl_find_object (String line, Object* container, ObjectLocation type, bool silent)
{
	const char* text = line.c_str();

	// check for arg
	if (str_empty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// ignore both 'my' and 'the'
	if (word_match (text, "my") || word_match(text, "the"))
		skip_word(text);
	
	// get index
	uint index = str_value(text);
	if (index)
		skip_word(text);
	else
		index = 1;

	// check for arg
	if (str_empty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// store name
	String name (text);

	// search room
	Object* object = container->find_object (name, index, type);
	if (object)
		return object;

	// type name
	*this << "You do not see '" << name << "' " << type.name() << " " << StreamName(container, DEFINITE) << ".\n";
	return NULL;
}


// parse a command line to get a creature
Creature* 
Creature::cl_find_creature (String line, bool silent)
{
	const char* text = line.c_str();

	// check for arg
	if (str_empty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	if (get_room() == NULL) {
		if (!silent)
			*this << "You are not in a room.\n";
		return NULL;
	}

	if (word_match(text, "the"))
		skip_word(text);

	uint index = str_value (text);
	if (index)
		skip_word(text);
	else
		index = 1;

	if (str_empty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	Creature* ch = get_room()->find_creature (String(text), index);
	if (ch == NULL && !silent)
		*this << "You do not see '" << text << "'.\n";

	return ch;
}

/* parse a command line to get an portal*/
Portal* 
Creature::cl_find_portal (String line, bool silent)
{
	const char* text = line.c_str();

	if (str_empty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	if (get_room() == NULL) {
		if (!silent)
			*this << "You are not in a room.\n";
		return NULL;
	}

	if (word_match(text, "the"))
		skip_word(text);

	uint index = str_value(text);
	if (index)
		skip_word(text);
	else
		index = 1;

	if (str_empty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	String name(text);
	Portal* portal;
	do {
		portal = get_room()->find_portal (name, index++);
	} while (portal != NULL && portal->is_disabled());

	if (portal == NULL && !silent) {
		*this << "You do not see '" << text << "'.\n";
	}

	return portal;
}

// parse a command line to get a creature, object, or portal
Entity* 
Creature::cl_find_any (String line, bool silent)
{
	uint matches;

	const char* text = line.c_str();

	// check for arg
	if (str_empty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// do we have a my keyword?
	if (word_match(text, "my")) {
		// do 'my' object search
		skip_word(text);
		return cl_find_object(String(text), GOC_EQUIP, silent);
	} else if (word_match(text, "the")) {
		skip_word(text);
	}

	// determine our index
	uint index = str_value(text);
	if (index)
		skip_word(text);
	else
		index = 1;

	// check for arg
	if (str_empty(text)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	String name(text);

	// look for a creature
	if (get_room()) {
		Creature* ch = get_room()->find_creature (name, index, &matches);
		if (ch != NULL)
			return ch;
		if (matches >= index) {
			if (!silent)
				*this << "You do not see '" << name << "'.\n";
			return NULL;
		}
		index -= matches;
	}

	// search room for object
	if (get_room() != NULL) {
		Object* obj = get_room()->find_object (name, index, &matches);
		if (obj)
			return obj;
		if (matches >= index) {
			if (!silent)
				*this << "You do not see '" << name << "'.\n";
			return NULL;
		}
		index -= matches; // update count; subtract matches

		// do a sub search for on containers, like tables
		for (EList<Object>::iterator iter = get_room()->objects.begin(); index > 0 && iter != get_room()->objects.end(); ++iter) {
			if (*iter != NULL) {
				// have an ON container - search inside
				if ((*iter)->has_location(ObjectLocation::ON)) {
					if ((obj = (*iter)->find_object (name, index, ObjectLocation::ON, &matches))) {
						return obj;
					}
					if (matches >= index) {
						*this << "You do not see '" << name << "'.\n";
						return NULL;
					}
					index -= matches; // update count; subtract matches
				}
			}
		}
	}

	// try held
	for (uint i = 0;; ++i) {
		Object* obj = get_held_at(i);
		// end of list?
		if (!obj)
			break;
		// matched and index was 1?
		if (obj->name_match(name)) {
			if(--index == 0)
				return obj;
		}
	}

	// try worn
	for (uint i = 0;; ++i) {
		Object* obj = get_worn_at(i);
		// end of list?
		if (!obj)
			break;
		// matched and index was 1?
		if (obj->name_match(name)) {
			if(--index == 0)
				return obj;
		}
	}

	// try an portal
	if (get_room()) {
		Portal* portal = NULL;
		do {
			portal = get_room()->find_portal (name, index++);
		} while (portal != NULL && portal->is_disabled());

		if (portal != NULL)
			return portal;
	}

	// print error
	if (!silent)
		*this << "You do not see '" << name << "'.\n";
	return NULL;
}

void
handle_char_move (Creature* ch, PortalDir dir) {
	// must be in a room
	if (!ch->get_room()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->get_room()->get_portal_by_dir (dir);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->do_go(portal);
}

// movement commands
void command_north (Creature* ch, String[]) { handle_char_move (ch, PortalDir::NORTH); }
void command_east (Creature* ch, String[]) { handle_char_move (ch, PortalDir::EAST); }
void command_south (Creature* ch, String[]) { handle_char_move (ch, PortalDir::SOUTH); }
void command_west (Creature* ch, String[]) { handle_char_move (ch, PortalDir::WEST); }
void command_northwest (Creature* ch, String[]) { handle_char_move (ch, PortalDir::NORTHWEST); }
void command_northeast (Creature* ch, String[]) { handle_char_move (ch, PortalDir::NORTHEAST); }
void command_southwest (Creature* ch, String[]) { handle_char_move (ch, PortalDir::SOUTHWEST); }
void command_southeast (Creature* ch, String[]) { handle_char_move (ch, PortalDir::SOUTHEAST); }

void command_go (Creature* ch, String argv[])
{
	if (ch->get_pos() != CreaturePosition::STAND) {
		*ch << "You cannot climb while " << ch->get_pos().get_verbing() << ".\n";
		return;
	}

	Portal* portal;
	if ((portal = ch->cl_find_portal (argv[0])) != NULL) {
		if (portal->get_usage() == PortalUsage::WALK) {
			ch->do_go (portal);
		} else {
			*ch << "You cannot do that with " << StreamName(*portal, DEFINITE) << ".\n";
		}
	}
}

void command_climb (Creature* ch, String argv[])
{
	if (ch->get_pos() != CreaturePosition::STAND) {
		*ch << "You cannot climb while " << ch->get_pos().get_verbing() << ".\n";
		return;
	}

	Portal* portal;
	if ((portal = ch->cl_find_portal (argv[0])) != NULL) {
		if (portal->get_usage() == PortalUsage::CLIMB) {
			ch->do_go (portal);
		} else {
			*ch << "You cannot climb " << StreamName(*portal, DEFINITE) << ".\n";
		}
	}
}

void command_crawl (Creature* ch, String argv[])
{
	if (ch->get_pos() != CreaturePosition::KNEEL && ch->get_pos() != CreaturePosition::SIT) {
		*ch << "You cannot crawl while " << ch->get_pos().get_verbing() << ".\n";
		return;
	}

	Portal* portal;
	if ((portal = ch->cl_find_portal (argv[0])) != NULL) {
		if (portal->get_usage() == PortalUsage::CRAWL) {
			ch->do_go (portal);
		} else {
			*ch << "You cannot crawl on " << StreamName(*portal, DEFINITE) << ".\n";
		}
	}
}

// basics
void command_look (Creature* ch, String argv[]) {
	// have we a target (argv[1])
	if (!argv[1]) {
		ch->do_look ();
		return;
	}

	// looking in/on/etc. container?
	if (argv[0] && argv[0] != "at") {
		Object* obj = ch->cl_find_object (argv[1], GOC_ANY);
		if (obj) {
			if (argv[0] == "on")
				ch->do_look (obj, ObjectLocation::ON);
			else if (argv[0] == "in")
				ch->do_look (obj, ObjectLocation::IN);
			else
				ch->do_look (obj, ObjectLocation::NONE);
		}
		return;
	}

	// generic find
	Entity* entity = ch->cl_find_any(argv[1]);
	if (entity != NULL) {
		// creature?
		if (CHARACTER(entity))
			ch->do_look((Creature*)(entity));
		// object?
		else if (OBJECT(entity))
			ch->do_look((Object*)(entity), ObjectLocation::NONE);
		// eixt?
		else if (PORTAL(entity))
			ch->do_look((Portal*)(entity));
	}
}

void command_say (Creature* ch, String argv[])
{
	ch->do_say (argv[0]);
}

void command_emote (Creature* ch, String argv[])
{
	ch->do_emote (argv[0]);
}

// position

void command_stand (Creature* ch, String[]) { ch->do_position (CreaturePosition::STAND); }
void command_sit (Creature* ch, String[]) { ch->do_position (CreaturePosition::SIT); }
void command_lay (Creature* ch, String[]) { ch->do_position (CreaturePosition::LAY); }
void command_kneel (Creature* ch, String[]) { ch->do_position (CreaturePosition::KNEEL); }

// interact

void command_open (Creature* ch, String argv[]) {
	Portal* portal;
	if ((portal = ch->cl_find_portal (argv[0])) != NULL) {
		if (portal->is_door ())
			ch->do_open (portal);
		else
			*ch << StreamName(portal, DEFINITE, true) << " is not a door.\n";
	}
}

void command_close (Creature* ch, String argv[]) {
	Portal* portal;
	if ((portal = ch->cl_find_portal (argv[0])) != NULL) {
		if (portal->is_door ())
			ch->do_close (portal);
		else
			*ch << StreamName(portal, DEFINITE, true) << " is not a door.\n";
	}
}

void command_lock (Creature* ch, String argv[]) {
	Portal* portal;
	if ((portal = ch->cl_find_portal (argv[0])) != NULL) {
		if (portal->is_door ())
			ch->do_lock (portal);
		else
			*ch << StreamName(portal, DEFINITE, true) << " is not a door.\n";
	}
}

void command_unlock (Creature* ch, String argv[]) {
	Portal* portal;
	if ((portal = ch->cl_find_portal (argv[0])) != NULL) {
		if (portal->is_door ())
			ch->do_unlock (portal);
		else
			*ch << StreamName(portal, DEFINITE, true) << " is not a door.\n";
	}
}

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

void command_put (Creature* ch, String argv[]) {
	Object* obj = ch->cl_find_object (argv[0], GOC_HELD);
	if (!obj) 
		return;

	Object* cobj = ch->cl_find_object (argv[2], GOC_ANY);
	if (!cobj)
		return;

	if (argv[1] == "on")
		ch->do_put (obj, cobj, ObjectLocation::ON);
	else if (argv[1] == "in")
		ch->do_put (obj, cobj, ObjectLocation::IN);
}

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

void command_wear (Creature* ch, String argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_HELD);
	if (obj)
		ch->do_wear (obj);
}

void command_equip (Creature* ch, String argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_HELD);
	if (obj)
		ch->do_wear (obj);
}

void command_remove (Creature* ch, String argv[]) 
{
	Object* obj = ch->cl_find_object (argv[0], GOC_WORN);
	if (obj)
		ch->do_remove (obj);
}

void command_read (Creature* ch, String argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_ANY);
	if (obj)
		ch->do_read (obj);
}

void command_kick (Creature* ch, String argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_ANY, true);
	if (obj) {
		ch->do_kick (obj);
		return;
	}
	Portal* portal = ch->cl_find_portal (argv[0]);
	if (portal) {
		ch->do_kick (portal);
		return;
	}
}

void command_raise (Creature* ch, String argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_HELD);
	if (obj)
		ch->do_raise (obj);
}

void command_eat (Creature* ch, String argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_HELD);
	if (obj)
		ch->do_eat (obj);
}

void command_drink (Creature* ch, String argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_ANY);
	if (obj)
		ch->do_drink (obj);
}

void command_swap (Creature* ch, String[])
{
	ch->swap_hands ();
	*ch << "You swap the contents of your hands.\n";
}

void command_touch (Creature* ch, String argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_ANY);
	if (obj)
		ch->do_touch (obj);
}

void command_sing (Creature* ch, String argv[])
{
	ch->do_sing(argv[0]);
}

void command_affects (Creature* ch, String argv[])
{
	ch->display_affects(*ch);
}

void command_stop (Creature* ch, String argv[])
{
	ch->cancel_action();
}
