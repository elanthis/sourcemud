/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */


#include "char.h"
#include "error.h"
#include "server.h"
#include "room.h"
#include "command.h"
#include "body.h"
#include "player.h"
#include "parse.h"
#include "streams.h"
#include "object.h"

void
Character::process_cmd (const char* line)
{
	assert (line != NULL);

	// check for talking with \", a special case
	if (line[0] == '\"' || line[0] == '\'')
	{
		if (line[1] != '\0')
			do_say (line+ 1);
		return;
	}

	// check for emote'ing with :, a special case
	if (line[0] == ':' || line[0] == ';')
	{
		if (line[1] != '\0')
			do_emote (line + 1);
		return;
	}

	CommandManager.call (this, line);
}

// parse a command line to get an object
Object* 
Character::cl_find_object (char* line, int type, bool silent)
{
	char* arg,* start,* name;
	Object* obj = NULL;
	uint index; // index to search at
	uint matches;

	start = line;

	// check for arg
	if (!commands::is_arg(line)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// do we have a my keyword?
	arg = commands::get_arg (&line);
	if (str_eq (arg, "my")) {
		type &= ~GOC_FLOOR; // clear floor flag
	} else if (!str_eq(arg, "the")) {
		commands::fix_arg (arg, &line);
	}

	// check for arg
	if (!commands::is_arg(line)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// determine our index
	arg = commands::get_arg (&line);
	if ((index = str_value (arg)) == 0) {
		index = 1;
		commands::fix_arg (arg, &line);
	}

	// check for arg
	if (!commands::is_arg(line)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// set name, fix up line
	name = line;
	commands::restore (start, &line);

	// search room
	if (type & GOC_FLOOR && get_room() != NULL) {
		obj = get_room()->find_object (name, index, &matches);
		if (obj)
			return obj;
		if (matches >= index) {
			*this << "You do not see '" << line << "'.\n";
			return NULL;
		}
		index -= matches; // update count; subtract matches
	}

	// do a sub search for on containers in the room, like tables
	if (type & GOC_SUB && get_room() != NULL) {
		for (EList<Object>::iterator iter = get_room()->objects.begin(); iter != get_room()->objects.end(); ++iter) {
			// have an ON container - search inside
			if (*iter != NULL && (*iter)->has_container (ContainerType::ON)) {
				if ((obj = (*iter)->find_object (name, index, ContainerType::ON, &matches))) {
					return obj;
				}
				if (matches >= index) {
					*this << "You do not see '" << line << "'.\n";
					return NULL;
				}
				index -= matches; // update count; subtract matches
			}
		}
	}

	// try held
	if (type & GOC_HELD)
		if ((obj = find_held (name)) != NULL)
			return obj;

	// try worn
	if (type & GOC_WORN)
		if ((obj = find_worn (name)) != NULL)
			return obj;

	// print error
	if (!silent) {
		if (type & GOC_ROOM)
			*this << "You do not see '" << line << "'.\n";
		else if ((type & GOC_HELD) && !(type & GOC_WORN))
			*this << "You are not holding '" << name << "'.\n";
		else if ((type & GOC_WORN) && !(type & GOC_HELD))
			*this << "You are not wearing '" << name << "'.\n";
		else if ((type & GOC_HELD) && (type & GOC_WORN))
			*this << "You don't have '" << name << "'.\n";
	}
	return NULL;
}

// parse a command line to get a character
Character* 
Character::cl_find_character (char* line, bool silent)
{
	char* arg,* name;
	Character* ch = NULL;
	uint index;

	// check for arg
	if (!commands::is_arg(line)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	if (get_room() == NULL) {
		if (!silent)
			*this << "You are not in a room.\n";
		return NULL;
	}

	arg = commands::get_arg (&line);
	if (str_eq(arg, "the"))
		arg = commands::get_arg (&line);
	if (!arg) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	if ((index = str_value (arg)) == 0) {
		commands::fix_arg (arg, &line);
		name = line;
		index = 1;
	} else {
		name = line;
		commands::fix_arg (arg, &line);
	}

	if ((ch = get_room()->find_character (name, index)) == NULL && !silent) {
		*this << "You do not see '" << line << "'.\n";
	}

	return ch;
}

/* parse a command line to get an exit*/
RoomExit* 
Character::cl_find_exit (char* line, bool silent)
{
	char* arg,* name;
	RoomExit* exit = NULL;
	uint index;

	// check for arg
	if (!commands::is_arg(line)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	if (get_room() == NULL) {
		if (!silent)
			*this << "You are not in a room.\n";
		return NULL;
	}

	arg = commands::get_arg (&line);
	if (str_eq(arg, "the"))
		arg = commands::get_arg (&line);
	if (!arg) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	if ((index = str_value (arg)) == 0) {
		commands::fix_arg (arg, &line);
		name = line;
		index = 1;
	} else {
		name = line;
		commands::fix_arg (arg, &line);
	}

	do {
		exit = get_room()->find_exit (name, index++);
	} while (exit != NULL && exit->is_disabled());

	if (exit == NULL && !silent) {
		*this << "You do not see '" << line << "'.\n";
	}

	return exit;
}

// parse a command line to get a character, object, or exit
Entity* 
Character::cl_find_any (char* line, bool silent)
{
	char* arg,* start,* name;
	uint index; // index to search at
	uint matches;

	start = line;

	// check for arg
	if (!commands::is_arg(line)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// do we have a my keyword?
	arg = commands::get_arg (&line);
	if (str_eq (arg, "my")) {
		// do 'my' object search
		name = line;
		commands::fix_arg (arg, &line);
		return cl_find_object(name, GOC_EQUIP, silent);
	} else if (!str_eq(arg, "the")) {
		commands::fix_arg (arg, &line);
	}

	// check for arg
	if (!commands::is_arg(line)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// determine our index
	arg = commands::get_arg (&line);
	if ((index = str_value (arg)) == 0) {
		index = 1;
		commands::fix_arg (arg, &line);
	}

	// check for arg
	if (!commands::is_arg(line)) {
		if (!silent)
			*this << "What?\n";
		return NULL;
	}

	// set name, fix up line
	name = line;
	commands::restore (start, &line);

	// look for a character
	if (get_room()) {
		Character* ch = get_room()->find_character (name, index, &matches);
		if (ch != NULL)
			return ch;
		if (matches >= index) {
			if (!silent)
				*this << "You do not see '" << line << "'.\n";
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
				*this << "You do not see '" << line << "'.\n";
			return NULL;
		}
		index -= matches; // update count; subtract matches

		// do a sub search for on containers, like tables
		for (EList<Object>::iterator iter = get_room()->objects.begin(); index > 0 && iter != get_room()->objects.end(); ++iter) {
			if (*iter != NULL) {
				// have an ON container - search inside
				if ((*iter)->has_container (ContainerType::ON)) {
					if ((obj = (*iter)->find_object (name, index, ContainerType::ON, &matches))) {
						return obj;
					}
					if (matches >= index) {
						*this << "You do not see '" << line << "'.\n";
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

	// try an exit
	if (get_room()) {
		RoomExit* exit = NULL;
		do {
			exit = get_room()->find_exit (name, index++);
		} while (exit != NULL && exit->is_disabled());

		if (exit != NULL)
			return exit;
	}

	// print error
	if (!silent)
		*this << "You do not see '" << line << "'.\n";
	return NULL;
}

void
handle_char_move (Character* ch, ExitDir dir) {
	// must be in a room
	if (!ch->get_room()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get exit
	RoomExit* exit = ch->get_room()->get_exit_by_dir (dir);
	if (!exit) {
		*ch << "You do not see an exit in that direction.\n";
		return;
	}

	// go
	ch->do_go(exit);
}

// movement commands
void command_north (Character* ch, char**) { handle_char_move (ch, ExitDir::NORTH); }
void command_east (Character* ch, char**) { handle_char_move (ch, ExitDir::EAST); }
void command_south (Character* ch, char**) { handle_char_move (ch, ExitDir::SOUTH); }
void command_west (Character* ch, char**) { handle_char_move (ch, ExitDir::WEST); }
void command_northwest (Character* ch, char**) { handle_char_move (ch, ExitDir::NORTHWEST); }
void command_northeast (Character* ch, char**) { handle_char_move (ch, ExitDir::NORTHEAST); }
void command_southwest (Character* ch, char**) { handle_char_move (ch, ExitDir::SOUTHWEST); }
void command_southeast (Character* ch, char**) { handle_char_move (ch, ExitDir::SOUTHEAST); }

void command_go (Character* ch, char** argv)
{
	if (ch->get_pos() != CharPosition::STAND) {
		*ch << "You cannot climb while " << ch->get_pos().get_verbing() << ".\n";
		return;
	}

	RoomExit* exit;
	if ((exit = ch->cl_find_exit (argv[0])) != NULL) {
		if (exit->get_usage() == ExitUsage::WALK) {
			ch->do_go (exit);
		} else {
			*ch << "You cannot do that with " << StreamName(*exit, DEFINITE) << ".\n";
		}
	}
}

void command_climb (Character* ch, char** argv)
{
	if (ch->get_pos() != CharPosition::STAND) {
		*ch << "You cannot climb while " << ch->get_pos().get_verbing() << ".\n";
		return;
	}

	RoomExit* exit;
	if ((exit = ch->cl_find_exit (argv[0])) != NULL) {
		if (exit->get_usage() == ExitUsage::CLIMB) {
			ch->do_go (exit);
		} else {
			*ch << "You cannot climb " << StreamName(*exit, DEFINITE) << ".\n";
		}
	}
}

void command_crawl (Character* ch, char** argv)
{
	if (ch->get_pos() != CharPosition::KNEEL && ch->get_pos() != CharPosition::SIT) {
		*ch << "You cannot crawl while " << ch->get_pos().get_verbing() << ".\n";
		return;
	}

	RoomExit* exit;
	if ((exit = ch->cl_find_exit (argv[0])) != NULL) {
		if (exit->get_usage() == ExitUsage::CRAWL) {
			ch->do_go (exit);
		} else {
			*ch << "You cannot crawl on " << StreamName(*exit, DEFINITE) << ".\n";
		}
	}
}

// basics
void command_look (Character* ch, char** argv) {
	// have we a target (argv[1])
	if (!argv[1]) {
		ch->do_look ();
		return;
	}

	// looking in/on/etc. container?
	if (argv[0] && !phrase_match("at", argv[0])) {
		ContainerType contain = ContainerType::lookup(argv[0]);
		if (contain.valid()) {
			Object* obj = ch->cl_find_object (argv[1], GOC_ANY);
			if (obj)
				ch->do_look (obj, contain);
			return;
		}
	}

	// generic find
	Entity* entity = ch->cl_find_any(argv[1]);
	if (entity != NULL) {
		// character?
		if (CHARACTER(entity))
			ch->do_look((Character*)(entity));
		// object?
		else if (OBJECT(entity))
			ch->do_look((Object*)(entity), ContainerType::NONE);
		// eixt?
		else if (ROOMEXIT(entity))
			ch->do_look((RoomExit*)(entity));
	}
}

void command_say (Character* ch, char** argv)
{
	ch->do_say (argv[0]);
}

void command_emote (Character* ch, char** argv)
{
	ch->do_emote (argv[0]);
}

// position

void command_stand (Character* ch, char**) { ch->do_position (CharPosition::STAND); }
void command_sit (Character* ch, char**) { ch->do_position (CharPosition::SIT); }
void command_lay (Character* ch, char**) { ch->do_position (CharPosition::LAY); }
void command_kneel (Character* ch, char**) { ch->do_position (CharPosition::KNEEL); }

// interact

void command_open (Character* ch, char** argv) {
	RoomExit* exit;
	if ((exit = ch->cl_find_exit (argv[0])) != NULL) {
		if (exit->is_door ())
			ch->do_open (exit);
		else
			*ch << StreamName(exit, DEFINITE, true) << " is not a door.\n";
	}
}

void command_close (Character* ch, char** argv) {
	RoomExit* exit;
	if ((exit = ch->cl_find_exit (argv[0])) != NULL) {
		if (exit->is_door ())
			ch->do_close (exit);
		else
			*ch << StreamName(exit, DEFINITE, true) << " is not a door.\n";
	}
}

void command_lock (Character* ch, char** argv) {
	RoomExit* exit;
	if ((exit = ch->cl_find_exit (argv[0])) != NULL) {
		if (exit->is_door ())
			ch->do_lock (exit);
		else
			*ch << StreamName(exit, DEFINITE, true) << " is not a door.\n";
	}
}

void command_unlock (Character* ch, char** argv) {
	RoomExit* exit;
	if ((exit = ch->cl_find_exit (argv[0])) != NULL) {
		if (exit->is_door ())
			ch->do_unlock (exit);
		else
			*ch << StreamName(exit, DEFINITE, true) << " is not a door.\n";
	}
}

void command_get (Character* ch, char** argv) {
	if (argv[2] != NULL) { // in, on, etc.
		ContainerType type = argv[1] ? ContainerType::lookup(argv[1]) : ContainerType::NONE;

		// get container
		Object* cobj = ch->cl_find_object (argv[2], GOC_ANY);
		if (!cobj)
			return;

		// no type, pick best, from in or on
		if (!type.valid()) {
			if (cobj->has_container (ContainerType::IN))
				type = ContainerType::IN;
			else if (cobj->has_container (ContainerType::ON))
				type = ContainerType::ON;
		} else if (!cobj->has_container (type)) {
			type = ContainerType::NONE; // invalidate type
		}
			
		// no valid type?
		if (!type.valid()) {
			*ch << "You can't do that with " << StreamName(*cobj, DEFINITE) << ".\n";
			return;
		}

		// determine count
		char* count = commands::get_arg(&argv[0]);
		if (str_eq(count, "the"))
			count = commands::get_arg(&argv[0]);
		uint index = str_value (count);
		if (index == 0) {
			index = 1;
			commands::fix_arg (count, &argv[0]);
		}

		// get object from container
		Object* obj = cobj->find_object (argv[0], index, type);
		if (obj)
			ch->do_get (obj, cobj, type);
		else
			*ch << "Can't find '" << argv[0] << "' " << type.get_name() << " " << StreamName(*cobj, DEFINITE) << ".\n";
	// get from the room
	} else {
		// coins?
		if (argv[0] == NULL) {
			Room* room = ch->get_room();
			if (room == NULL) {
				*ch << "You are not in a room.\n";
				return;
			}

			uint max = room->get_coins();
			int amount = 0;

			// how many?
			if (argv[3] == NULL) {
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
				ch->do_get (obj, NULL, 0);
		}
	}
}

void command_put (Character* ch, char** argv) {
	ContainerType type = ContainerType::lookup(argv[1]);

	Object* obj = ch->cl_find_object (argv[0], GOC_HELD);
	if (!obj) 
		return;

	Object* cobj = ch->cl_find_object (argv[2], GOC_ANY);
	if (!cobj)
		return;

	ch->do_put (obj, cobj, type);
}

void command_give (Character* ch, char** argv)
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
	Character* target = ch->cl_find_character(argv[1]);
	if (!target)
		return;

	// do give
	ch->do_give_coins (target, amount);
}

void command_drop (Character* ch, char** argv)
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

void command_wear (Character* ch, char** argv)
{
	Object* obj = ch->cl_find_object (argv[0], GOC_HELD);
	if (obj)
		ch->do_wear (obj);
}

void command_equip (Character* ch, char** argv)
{
	Object* obj = ch->cl_find_object (argv[0], GOC_HELD);
	if (obj)
		ch->do_wear (obj);
}

void command_remove (Character* ch, char** argv) 
{
	Object* obj = ch->cl_find_object (argv[0], GOC_WORN);
	if (obj)
		ch->do_remove (obj);
}

void command_read (Character* ch, char** argv)
{
	Object* obj = ch->cl_find_object (argv[0], GOC_ANY);
	if (obj)
		ch->do_read (obj);
}

void command_kick (Character* ch, char** argv)
{
	Object* obj = ch->cl_find_object (argv[0], GOC_ANY, true);
	if (obj) {
		ch->do_kick (obj);
		return;
	}
	RoomExit* exit = ch->cl_find_exit (argv[0]);
	if (exit) {
		ch->do_kick (exit);
		return;
	}
}

void command_raise (Character* ch, char** argv)
{
	Object* obj = ch->cl_find_object (argv[0], GOC_HELD);
	if (obj)
		ch->do_raise (obj);
}

void command_eat (Character* ch, char** argv)
{
	Object* obj = ch->cl_find_object (argv[0], GOC_HELD);
	if (obj)
		ch->do_eat (obj);
}

void command_drink (Character* ch, char** argv)
{
	Object* obj = ch->cl_find_object (argv[0], GOC_ANY);
	if (obj)
		ch->do_drink (obj);
}

void command_swap (Character* ch, char**)
{
	ch->swap_hands ();
	*ch << "You swap the contents of your hands.\n";
}

void command_touch (Character* ch, char** argv)
{
	Object* obj = ch->cl_find_object (argv[0], GOC_ANY);
	if (obj)
		ch->do_touch (obj);
}

void command_sing (Character* ch, char** argv)
{
	ch->do_sing(argv[0]);
}

void command_affects (Character* ch, char** argv)
{
	ch->display_affects(*ch);
}

void command_stop (Character* ch, char** argv)
{
	ch->cancel_action();
}
