/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "mud/entity.h"
#include "mud/char.h"
#include "common/error.h"
#include "mud/server.h"
#include "common/string.h"
#include "mud/body.h"
#include "mud/room.h"
#include "common/rand.h"
#include "mud/player.h"
#include "mud/parse.h"
#include "common/streams.h"
#include "mud/eventids.h"
#include "mud/action.h"
#include "mud/caffect.h"
#include "mud/clock.h"
#include "mud/object.h"
#include "mud/hooks.h"

// ----- CharStatID -----

String CharStatID::names[CharStatID::COUNT] = {
	S("Strength"),
	S("Agility"),
	S("Fortitude"),
	S("Intellect"),
	S("Spirit"),
	S("Willpower"),
};

CharStatID
CharStatID::lookup (StringArg name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (str_eq(name, names[i]))
			return i;
	return NONE;
}

String stat_levels[] = {
	S("Wretched"),
	S("Horrible"),
	S("Bad"),
	S("Poor"),
	S("Average"),
	S("Fair"),
	S("Good"),
	S("Excellent"),
	S("Awesome"),
};

String
get_stat_level (uint stat) {
	if (stat <= 15)
		return stat_levels[0];
	else if (stat <= 25)
		return stat_levels[1];
	else if (stat <= 35)
		return stat_levels[2];
	else if (stat <= 45)
		return stat_levels[3];
	else if (stat <= 55)
		return stat_levels[4];
	else if (stat <= 65)
		return stat_levels[5];
	else if (stat <= 75)
		return stat_levels[6];
	else if (stat <= 85)
		return stat_levels[7];
	else
		return stat_levels[8];
}

const char *
get_stat_color (uint stat) {
	if (stat <= 35)
		return CSTAT_BAD2;
	else if (stat <= 45)
		return CSTAT_BAD1;
	else if (stat <= 55)
		return CSTAT;
	else if (stat <= 65)
		return CSTAT_GOOD1;
	else
		return CSTAT_GOOD2;
}

// ----- CharPosition -----

String CharPosition::names[CharPosition::COUNT] = {
	S("stand"),
	S("sit"),
	S("lay"),
	S("kneel"),
};
String CharPosition::verbs[CharPosition::COUNT] = {
	S("stand up"),
	S("sit down"),
	S("lay down"),
	S("kneel"),
};
String CharPosition::sverbs[CharPosition::COUNT] = {
	S("stands up"),
	S("sits down"),
	S("lays down"),
	S("kneels"),
};
String CharPosition::verbings[CharPosition::COUNT] = {
	S("standing"),
	S("sitting"),
	S("laying down"),
	S("kneeling"),
};

CharPosition
CharPosition::lookup (StringArg name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (str_eq(name, names[i]))
			return i;
	return STAND;
}

// ----- Character -----

void
Character::save (File::Writer& writer)
{
	Entity::save(writer);

	if (dead)
		writer.attr(S("dead"), S("yes"));

	writer.attr(S("position"), position.get_name());

	if (coins)
		writer.attr(S("coins"), coins);

	writer.attr(S("hp"), health.cur);

	if (equipment.right_held) {
		writer.begin(S("equip.right_hand"));
		equipment.right_held->save(writer);
		writer.end();
	}
	if (equipment.left_held) {
		writer.begin(S(S("equip.left_hand")));
		equipment.left_held->save(writer);
		writer.end();
	}
	if (equipment.body_worn) {
		writer.begin(S("equip.body"));
		equipment.body_worn->save(writer);
		writer.end();
	}
	if (equipment.back_worn) {
		writer.begin(S("equip.back"));
		equipment.back_worn->save(writer);
		writer.end();
	}
	if (equipment.waist_worn) {
		writer.begin(S("equip.waist"));
		equipment.waist_worn->save(writer);
		writer.end();
	}
}

void
Character::save_hook (ScriptRestrictedWriter* writer)
{
	Entity::save_hook(writer);
	Hooks::save_character(this, writer);
}

int
Character::load_node (File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_PARENT(Entity)
		FO_ATTR("dead")
			dead = node.get_data();
		FO_ATTR("position")
			position = CharPosition::lookup(node.get_data());
		FO_ATTR("coins")
			FO_TYPE_ASSERT(INT);
			coins = tolong(node.get_data());
		FO_ATTR("hp")
			FO_TYPE_ASSERT(INT);
			health.cur = tolong(node.get_data());
		FO_OBJECT("equip.right_hand")
			equipment.right_held = new Object();
			if (equipment.right_held->load (reader))
				throw "Failed to load object";
			equipment.right_held->set_owner(this);
		FO_OBJECT("equip.left_hand")
			equipment.left_held = new Object();
			if (equipment.left_held->load (reader))
				throw "Failed to load object";
			equipment.left_held->set_owner(this);
		FO_OBJECT("equip.body")
			equipment.body_worn = new Object();
			if (equipment.body_worn->load (reader))
				throw "Failed to load object";
			equipment.body_worn->set_owner(this);
		FO_OBJECT("equip.back")
			equipment.back_worn = new Object();
			if (equipment.back_worn->load (reader))
				throw "Failed to load object";
			equipment.back_worn->set_owner(this);
		FO_OBJECT("equip.waist")
			equipment.waist_worn = new Object();
			if (equipment.waist_worn->load (reader))
				throw "Failed to load object";
			equipment.waist_worn->set_owner(this);
	FO_NODE_END
}

int
Character::load_finish (void)
{
	recalc();

	return 0;
}

// Character
Character::Character (const Scriptix::TypeInfo* type) : Entity (type)
{
	position = CharPosition::STAND;
	location = NULL;
	health.cur = health.max = 0;
	round_time = 0;
	coins = 0;
	dead = false;

	equipment.right_held = NULL;
	equipment.left_held = NULL;
	equipment.body_worn = NULL;
	equipment.waist_worn = NULL;
	equipment.back_worn = NULL;

	for (int i = 0; i < CharStatID::COUNT; ++ i)
		effective_stats[i] = 0;
}

void
Character::set_owner (Entity* s_owner)
{
	// type check
	assert(ROOM(s_owner));

	// set owner
	Entity::set_owner(s_owner);
	location = (Room*)s_owner;
}

Entity*
Character::get_owner (void) const
{
	return location;
}

// add an action
void
Character::add_action (IAction* action)
{
	// insert action before this point
	actions.push_back(action);

	// only action on list?  initialize it
	if (actions.size() == 1) {
		round_time = 0;
		if (action->start() != 0)
			actions.erase(actions.begin());
	}
}

IAction*
Character::get_action (void) const
{
	if (actions.empty())
		return NULL;
	else
		return actions.front();
}

void
Character::cancel_action (void)
{
	// no actions?  blegh
	if (actions.empty()) {
		*this << "You are not doing anything to stop.\n";
		return;
	}

	// try cancelletion
	if (actions.front()->cancel() != 0) {
		*this << "You can't stop ";
		actions.front()->describe(*this);
		*this << ".\n";
		return;
	}

	// reset round time
	round_time = 0;

	// keep looping until we have an action that doesn't
	// abort, or we are out of actions
	actions.erase(actions.begin());
	while (!actions.empty() && actions.front()->start() != 0)
		actions.erase(actions.begin());
}

// get the round time
uint
Character::get_round_time (void) const
{
	// no actions?  no round time
	if (actions.empty())
		return 0;

	/*
	uint rounds = 0;

	for (ActionList::const_iterator i = actions.begin(); i != actions.end(); ++i)
		rounds += (*i)->get_rounds();
	*/
	uint rounds = actions.front()->get_rounds();

	if (rounds < round_time)
		return 0;
	return rounds - round_time;
}

bool
Character::check_alive (void) {
	if (is_dead()) {
		*this << "You are only a ghost.\n";
		return false;
	}
	return true;
}

bool
Character::check_move (void) {
	// can't move if you're dead
	if (!check_alive())
		return false;

	if (!can_move()) {
		*this << "You cannot move.\n";
		return false;
	}

	return true;
}

bool
Character::check_see (void) {
	if (!can_see()) {
		*this << "You cannot see.\n";
		return false;
	}
	return true;
}

bool
Character::check_rt (void) {
	// round time?
	uint rounds = get_round_time();
	
	if (rounds > 0) {
		// action?
		if (!actions.empty()) {
			*this << "You must wait " << rounds << " second" << (rounds > 1 ? "s" : "") << " until you are done ";
			actions.front()->describe(*this);
			*this << ".\n";
		} else {
			*this << "You must wait " << rounds << " second" << (rounds > 1 ? "s" : "") << " for your round time to expire.\n";
		}

		return false;
	}
	return true;
}

// -------------------- FUNCS ----------------------

// move into a new room
bool
Character::enter (Room *new_room, RoomExit *old_exit)
{
	assert (new_room != NULL);

	// already here
	if (new_room->chars.has(this))
		return false;

	// entering exit
	RoomExit* enter_exit = NULL;

	// did we go thru an exit?
	if (old_exit) {
		// "You go..." message
		*this << StreamParse(old_exit->get_go()).add(S("actor"), this).add(S("exit"), old_exit) << "\n";

		// "So-and-so leaves thru..." message
		if (get_room())
			*get_room() << StreamIgnore(this) << StreamParse(old_exit->get_leaves()).add(S("actor"), this).add( S("exit"), old_exit) << "\n";

		// opposite exit is our entrance
		enter_exit = new_room->get_exit_by_dir(old_exit->get_dir().get_opposite());
	}

	// valid exit?
	if (enter_exit)
		*new_room << StreamParse(enter_exit->get_enters()).add(S("actor"), this).add(S("exit"), enter_exit) << "\n";
	else
		*new_room << StreamName(this, INDEFINITE, true) << " arrives.\n";

	// move, look, event
	Events::send_leave(get_room(), this, old_exit);
	new_room->add_character (this);
	do_look();
	Events::send_enter(get_room(), this, enter_exit);

	return true;
}

void
Character::heal (uint amount)
{
	bool was_dead = is_dead();
	health.cur += amount;
	int max = get_max_hp();
	if (health.cur > max)
		health.cur = max;
	// have we been resurrected?
	if (health.cur > 0 && was_dead) {
		dead = false;
		Events::send_resurrect(get_room(), this);
	}
}

bool
Character::damage (uint amount, Character *trigger) {
	// already dead?  no reason to continue
	if (is_dead())
		return false;
	// do damage and event
	health.cur -= amount;
	Events::send_damage(get_room(), this, trigger, amount);
	// caused death?
	if (health.cur <= 0 && !is_dead ()) {
		dead = true;
		kill (trigger);
		return true;
	}
	// still alive
	return false;
}

uint
Character::give_coins (uint amount)
{
	uint space = UINT_MAX - coins;
	if (space < amount)
		return coins += space;
	else
		return coins += amount;
}

uint
Character::take_coins (uint amount)
{
	if (amount > coins)
		return coins = 0;
	else
		return coins -= amount;
}

void
Character::heartbeat (void)
{
	// pending actions?
	if (!actions.empty()) {
		// one round
		++round_time;

		// last round?
		bool done = false;
		if (round_time >= actions.front()->get_rounds()) {
			// finish it up
			actions.front()->finish();

			// done
			done = true;
		// not the last round
		} else {
			// update it
			done = actions.front()->update(round_time);
		}

		// all done?  find next!
		if (done) {
			// reset round
			round_time = 0;

			// find next valid
			do
				actions.erase(actions.begin());
			while (!actions.empty() && actions.front()->start() != 0);
		}
	}

	// healing
	if (!is_dead() && (AweMUD::get_rounds() % (50 - get_effective_stat(CharStatID::FORTITUDE) / 5)) == 0) {
		heal(1);
	}

	// affects
	for (AffectStatusList::iterator i = affects.begin(); i != affects.end();) {
		(*i)->update(this);

		// affect expire?
		if ((*i)->get_time_left() == 0) {
			(*i)->remove(this);
			i = affects.erase(i);
		} else {
			++i;
		}
	}

	// update handler
	Hooks::character_heartbeat(this);
}

void
Character::activate (void)
{
	Entity::activate();

	Object* obj;
	for (int i = 0; (obj = get_equip_at(i)) != NULL; ++i)
		obj->activate();
}

void
Character::deactivate (void)
{
	Object* obj;
	for (int i = 0; (obj = get_equip_at(i)) != NULL; ++i)
		obj->deactivate();

	Entity::deactivate();
}

int
Character::parse_property (const StreamControl& stream, StringArg comm, const ParseArgs& argv) const
{
	// HE / SHE
	if (str_eq(comm, S("he"))) {
		stream << get_gender().get_heshe();
	}
	// HIM / HER
	else if (str_eq(comm, S("him"))) {
		stream << get_gender().get_himher();
	}
	// HIS / HER
	else if (str_eq(comm, S("his"))) {
		stream << get_gender().get_hisher();
	}
	// HIS / HERS
	else if (str_eq(comm, S("hers"))) {
		stream << get_gender().get_hishers();
	}
	// MAN / WOMAN
	else if (str_eq(comm, S("man"))) {
		stream << get_gender().get_manwoman();
	}
	// MALE / FEMALE
	else if (str_eq(comm, S("male"))) {
		stream << get_gender().get_malefemale();
	}
	// ALIVE / DEAD
	else if (str_eq(comm, S("alive"))) {
		if (is_dead())
			stream << "dead";
		else
			stream << "alive";
	}
	// POSITION
	else if (str_eq(comm, S("position"))) {
		stream << get_pos().get_verbing();
	}
	// default...
	else {
		return Entity::parse_property(stream, comm, argv);
	}

	return 0;
}

// recalc stats
void
Character::recalc_stats (void)
{
	for (int i = 0; i < CharStatID::COUNT; ++i)
		effective_stats[i] = get_base_stat(i);
}

// recalc max health
void
Character::recalc_health (void)
{
	health.max = (10 + get_stat_modifier(CharStatID::FORTITUDE)) * 10;

	// cap HP
	if (health.cur > health.max)
		health.cur = health.max;
}

// recalculate various stuff
void
Character::recalc (void)
{
	recalc_stats();
	recalc_health();
}

void
Character::display_equip (const StreamControl& stream) const
{
	// inventory variables
	uint loc = 0;
	Object* obj;
	Object* last = NULL;
	bool didshow = false;

	// worn items
	while ((obj = get_worn_at(loc++)) != NULL) {
		// hidden?  skip
		if (obj->is_hidden())
			continue;
		// we had one already?
		if (last) {
			// prefix
			if (didshow) {
				stream << ", ";
			} else {
				stream << StreamParse(S("  {$1.He} is wearing "), S("self"), this);
				didshow = true;
			}
			// do show
			stream << StreamName(last, INDEFINITE);
		}
		// remember this object
		last = obj;
	}
	// spit out the left over
	if (last) {
		// prefix
		if (didshow) {
			stream << " and ";
		} else {
			stream << StreamParse(S("  {$1.He} is wearing "), S("self"), this);
			didshow = true;
		}
		// show it
		stream << StreamName(last, INDEFINITE) << ".";
	}

	// held items
	loc = 0;
	didshow = false;
	last = NULL;
	while ((obj = get_held_at(loc++)) != NULL) {
		// hidden?  skip
		if (obj->is_hidden())
			continue;
		// we had one already?
		if (last) {
			// prefix
			if (didshow) {
				stream << ", ";
			} else {
				stream << StreamParse(S("  {$1.He} is holding "), S("self"), this);
				didshow = true;
			}
			// show
			stream << StreamName(last, INDEFINITE);
		}
		last = obj;
	}
	// show the last one
	if (last) {
		// prefix
		if (didshow) {
			stream << " and ";
		} else {
			stream << StreamParse(S("  {$1.He} is holding "), S("self"), this);
			didshow = true;
		}
		// show it
		stream << StreamName(last, INDEFINITE) << ".";
	}

	// dead or position
	if (is_dead())
		stream << StreamParse(S("  {$1.He} is laying on the ground, dead."), S("self"), this);
	else if (get_pos() != CharPosition::STAND)
		stream << StreamParse(S("  {$1.He} is {$1.position}."), S("self"), this);

	// health
	if (!is_dead() && get_max_hp() > 0) {
		if (get_hp() * 100 / get_max_hp() <= 25)
			stream << StreamParse(S("  {$1.He} appears severely wounded."), S("self"), this);
		else if (get_hp() * 100 / get_max_hp() <= 75)
			stream << StreamParse(S("  {$1.He} appears wounded."), S("self"), this);
		else
			stream << StreamParse(S("  {$1.He} appears to be in good health."), S("self"), this);
	}
}

void
Character::display_affects (const StreamControl& stream) const
{
	bool found = false;
	for (AffectStatusList::const_iterator i = affects.begin(); i != affects.end(); ++i) {
		if (!(*i)->get_title().empty()) {
			if (!found) {
				found = true;
				stream << "Active affects:\n";
			}
			stream << "  " << (*i)->get_title() << " [" << (*i)->get_type().get_name() << "]\n";
		}
	}

	if (!found)
		stream << "There are no active affects.\n";
}

// stat modifier
int
Character::get_stat_modifier (CharStatID stat) const
{
	assert(stat);

	return (get_effective_stat(stat) - 50) / 10;
}

// add an affect
int
Character::add_affect (CharacterAffectGroup* affect)
{
	if (affect->apply(this))
		return -1;

	affects.push_back(affect);
	return 0;
}
