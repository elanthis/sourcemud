/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <climits>

#include "mud/entity.h"
#include "mud/creature.h"
#include "common/error.h"
#include "mud/server.h"
#include "common/string.h"
#include "mud/body.h"
#include "mud/room.h"
#include "common/rand.h"
#include "mud/player.h"
#include "mud/macro.h"
#include "common/streams.h"
#include "mud/action.h"
#include "mud/caffect.h"
#include "mud/clock.h"
#include "mud/object.h"
#include "generated/hooks.h"
#include "mud/shadow-object.h"
#include "mud/unique-object.h"

// ----- CreatureStatID -----

String CreatureStatID::names[CreatureStatID::COUNT] = {
	S("Strength"),
	S("Agility"),
	S("Fortitude"),
	S("Intellect"),
	S("Spirit"),
	S("Willpower"),
};

String CreatureStatID::short_names[CreatureStatID::COUNT] = {
	S("ST"),
	S("AG"),
	S("FO"),
	S("IN"),
	S("SP"),
	S("WI"),
};

CreatureStatID
CreatureStatID::lookup (String name)
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

String
get_stat_color (uint stat) {
	if (stat <= 35)
		return S(CSTAT_BAD2);
	else if (stat <= 45)
		return S(CSTAT_BAD1);
	else if (stat <= 55)
		return S(CSTAT);
	else if (stat <= 65)
		return S(CSTAT_GOOD1);
	else
		return S(CSTAT_GOOD2);
}

// ----- CreaturePosition -----

String CreaturePosition::names[CreaturePosition::COUNT] = {
	S("stand"),
	S("sit"),
	S("lay"),
	S("kneel"),
};
String CreaturePosition::verbs[CreaturePosition::COUNT] = {
	S("stand up"),
	S("sit down"),
	S("lay down"),
	S("kneel"),
};
String CreaturePosition::sverbs[CreaturePosition::COUNT] = {
	S("stands up"),
	S("sits down"),
	S("lays down"),
	S("kneels"),
};
String CreaturePosition::verbings[CreaturePosition::COUNT] = {
	S("standing"),
	S("sitting"),
	S("laying down"),
	S("kneeling"),
};

CreaturePosition
CreaturePosition::lookup (String name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (str_eq(name, names[i]))
			return i;
	return STAND;
}

// ----- Creature -----

void
Creature::save_data (File::Writer& writer)
{
	Entity::save_data(writer);

	if (dead)
		writer.attr(S("creature"), S("dead"), S("yes"));

	writer.attr(S("creature"), S("position"), position.get_name());

	if (coins)
		writer.attr(S("creature"), S("coins"), coins);

	writer.attr(S("creature"), S("hp"), health.cur);

	if (equipment.right_held)
		equipment.right_held->save(writer, S("creature"), S("equip_rhand"));
	if (equipment.left_held)
		equipment.left_held->save(writer, S("creature"), S("equip_lhand"));
	if (equipment.body_worn)
		equipment.body_worn->save(writer, S("creature"), S("equip_body"));
	if (equipment.back_worn)
		equipment.back_worn->save(writer, S("creature"), S("equip_back"));
	if (equipment.waist_worn)
		equipment.waist_worn->save(writer, S("creature"), S("equip_waist"));
}

void
Creature::save_hook (ScriptRestrictedWriter* writer)
{
	Entity::save_hook(writer);
	Hooks::save_creature(this, writer);
}

int
Creature::load_node (File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_PARENT(Entity)
		FO_ATTR("creature", "dead")
			dead = node.get_string();
		FO_ATTR("creature", "position")
			position = CreaturePosition::lookup(node.get_string());
		FO_ATTR("creature", "coins")
			coins = node.get_int();
		FO_ATTR("creature", "hp")
			health.cur = node.get_int();

		FO_ENTITY("creature", "equip_rhand")
			if (OBJECT(entity) == NULL) throw File::Error(S("Equipment is not an Object"));
			equipment.right_held = OBJECT(entity);
			equipment.right_held->set_owner(this);
		FO_ENTITY("creature", "equip_lhand")
			if (OBJECT(entity) == NULL) throw File::Error(S("Equipment is not an Object"));
			equipment.left_held = OBJECT(entity);
			equipment.left_held->set_owner(this);
		FO_ENTITY("creature", "equip_body")
			if (OBJECT(entity) == NULL) throw File::Error(S("Equipment is not an Object"));
			equipment.body_worn = OBJECT(entity);
			equipment.body_worn->set_owner(this);
		FO_ENTITY("creature", "equip_back")
			if (OBJECT(entity) == NULL) throw File::Error(S("Equipment is not an Object"));
			equipment.back_worn = OBJECT(entity);
			equipment.back_worn->set_owner(this);
		FO_ENTITY("creature", "equip_waist")
			if (OBJECT(entity) == NULL) throw File::Error(S("Equipment is not an Object"));
			equipment.waist_worn = OBJECT(entity);
			equipment.waist_worn->set_owner(this);
	FO_NODE_END
}

int
Creature::load_finish (void)
{
	recalc();

	return 0;
}

// Creature
Creature::Creature (const Scriptix::TypeInfo* type) : Entity (type)
{
	position = CreaturePosition::STAND;
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

	for (int i = 0; i < CreatureStatID::COUNT; ++ i)
		effective_stats[i] = 0;
}

void
Creature::set_owner (Entity* s_owner)
{
	// type check
	assert(ROOM(s_owner));

	// set owner
	Entity::set_owner(s_owner);
	location = (Room*)s_owner;
}

Entity*
Creature::get_owner (void) const
{
	return location;
}

// add an action
void
Creature::add_action (IAction* action)
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
Creature::get_action (void) const
{
	if (actions.empty())
		return NULL;
	else
		return actions.front();
}

void
Creature::cancel_action (void)
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
Creature::get_round_time (void) const
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
Creature::check_alive (void) {
	if (is_dead()) {
		*this << "You are only a ghost.\n";
		return false;
	}
	return true;
}

bool
Creature::check_move (void) {
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
Creature::check_see (void) {
	if (!can_see()) {
		*this << "You cannot see.\n";
		return false;
	}
	return true;
}

bool
Creature::check_rt (void) {
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
Creature::enter (Room *new_room, Portal *old_portal)
{
	assert (new_room != NULL);
	Room* old_room = get_room();

	// already here
	if (new_room->creatures.has(this))
		return false;

	// entering portal
	Portal* enter_portal = NULL;
	if (old_portal && old_room)
		enter_portal = old_portal->get_relative_portal(old_room);
	
	// zones
	Zone* old_zone = NULL;
	if (old_room)
		old_zone = old_room->get_zone();
	Zone* new_zone = new_room->get_zone();

	// did we go thru an portal?
	if (old_portal) {
		// "You go..." message
		*this << StreamMacro(old_portal->get_go()).add(S("actor"), this).add(S("portal"), old_portal) << "\n";

		// "So-and-so leaves thru..." message
		if (old_room)
			*old_room << StreamIgnore(this) << StreamMacro(old_portal->get_leaves()).add(S("actor"), this).add( S("portal"), old_portal) << "\n";
	}

	// valid portal?
	if (enter_portal)
		*new_room << StreamMacro(enter_portal->get_enters()).add(S("actor"), this).add(S("portal"), enter_portal) << "\n";
	else
		*new_room << StreamName(this, INDEFINITE, true) << " arrives.\n";

	new_room->add_creature (this);

	if (old_room)
		Events::sendLeaveRoom(old_room, this, old_portal, new_room);
	Events::sendEnterRoom(new_room, this, enter_portal, old_room);
	if (old_room && old_zone != new_zone)
		Events::sendLeaveZone(old_room, this, new_zone);
	if (old_room && old_zone != new_zone)
		Events::sendEnterZone(old_room, this, old_zone);

	do_look();

	return true;
}

void
Creature::heal (uint amount)
{
	bool was_dead = is_dead();
	health.cur += amount;
	int max = get_max_hp();
	if (health.cur > max)
		health.cur = max;
	// have we been resurrected?
	if (health.cur > 0 && was_dead) {
		dead = false;
		// FIXME EVENT
	}
}

bool
Creature::damage (uint amount, Creature *trigger) {
	// already dead?  no reason to continue
	if (is_dead())
		return false;
	// do damage and event
	health.cur -= amount;
	// FIXME EVENT
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
Creature::give_coins (uint amount)
{
	uint space = UINT_MAX - coins;
	if (space < amount)
		return coins += space;
	else
		return coins += amount;
}

uint
Creature::take_coins (uint amount)
{
	if (amount > coins)
		return coins = 0;
	else
		return coins -= amount;
}

void
Creature::heartbeat (void)
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
	if (!is_dead() && (AweMUD::get_rounds() % (50 - get_effective_stat(CreatureStatID::FORTITUDE) / 5)) == 0) {
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
	Hooks::creature_heartbeat(this);
}

void
Creature::activate (void)
{
	Entity::activate();

	Object* obj;
	for (int i = 0; (obj = get_equip_at(i)) != NULL; ++i)
		obj->activate();
}

void
Creature::deactivate (void)
{
	Object* obj;
	for (int i = 0; (obj = get_equip_at(i)) != NULL; ++i)
		obj->deactivate();

	Entity::deactivate();
}

int
Creature::macro_property (const StreamControl& stream, String comm, const MacroList& argv) const
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
		return Entity::macro_property(stream, comm, argv);
	}

	return 0;
}

// recalc stats
void
Creature::recalc_stats (void)
{
	for (int i = 0; i < CreatureStatID::COUNT; ++i)
		effective_stats[i] = get_base_stat(i);
}

// recalc max health
void
Creature::recalc_health (void)
{
	health.max = (10 + get_stat_modifier(CreatureStatID::FORTITUDE)) * 10;

	// cap HP
	if (health.cur > health.max)
		health.cur = health.max;
}

// recalculate various stuff
void
Creature::recalc (void)
{
	recalc_stats();
	recalc_health();
}

void
Creature::display_equip (const StreamControl& stream) const
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
				stream << StreamMacro(S("  {$self.He} is wearing "), S("self"), this);
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
			stream << StreamMacro(S("  {$self.He} is wearing "), S("self"), this);
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
				stream << StreamMacro(S("  {$self.He} is holding "), S("self"), this);
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
			stream << StreamMacro(S("  {$self.He} is holding "), S("self"), this);
			didshow = true;
		}
		// show it
		stream << StreamName(last, INDEFINITE) << ".";
	}

	// dead or position
	if (is_dead())
		stream << StreamMacro(S("  {$self.He} is laying on the ground, dead."), S("self"), this);
	else if (get_pos() != CreaturePosition::STAND)
		stream << StreamMacro(S("  {$self.He} is {$self.position}."), S("self"), this);

	// health
	if (!is_dead() && get_max_hp() > 0) {
		if (get_hp() * 100 / get_max_hp() <= 25)
			stream << StreamMacro(S("  {$self.He} appears severely wounded."), S("self"), this);
		else if (get_hp() * 100 / get_max_hp() <= 75)
			stream << StreamMacro(S("  {$self.He} appears wounded."), S("self"), this);
		else
			stream << StreamMacro(S("  {$self.He} appears to be in good health."), S("self"), this);
	}
}

void
Creature::display_affects (const StreamControl& stream) const
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
Creature::get_stat_modifier (CreatureStatID stat) const
{
	assert(stat);

	return (get_effective_stat(stat) - 50) / 10;
}

// add an affect
int
Creature::add_affect (CreatureAffectGroup* affect)
{
	if (affect->apply(this))
		return -1;

	affects.push_back(affect);
	return 0;
}

// events
void
Creature::handle_event (const Event& event)
{
	Entity::handle_event(event);
}

void
Creature::broadcast_event (const Event& event)
{
	if (equipment.right_held)
		EventManager.resend(event, equipment.right_held);
	if (equipment.left_held)
		EventManager.resend(event, equipment.left_held);
	if (equipment.body_worn)
		EventManager.resend(event, equipment.body_worn);
	if (equipment.back_worn)
		EventManager.resend(event, equipment.back_worn);
	if (equipment.waist_worn)
		EventManager.resend(event, equipment.waist_worn);
}
