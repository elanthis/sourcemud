/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/error.h"
#include "common/string.h"
#include "common/rand.h"
#include "common/streams.h"
#include "mud/entity.h"
#include "mud/creature.h"
#include "mud/server.h"
#include "mud/body.h"
#include "mud/room.h"
#include "mud/player.h"
#include "mud/macro.h"
#include "mud/action.h"
#include "mud/caffect.h"
#include "mud/clock.h"
#include "mud/object.h"
#include "mud/hooks.h"

// ----- CreatureStatID -----

std::string CreatureStatID::names[CreatureStatID::COUNT] = {
	"Strength",
	"Agility",
	"Fortitude",
	"Intellect",
	"Spirit",
	"Willpower",
};

std::string CreatureStatID::short_names[CreatureStatID::COUNT] = {
	"ST",
	"AG",
	"FO",
	"IN",
	"SP",
	"WI",
};

CreatureStatID CreatureStatID::lookup(const std::string& name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (strEq(name, names[i]))
			return i;
	return NONE;
}

std::string stat_levels[] = {
	"Wretched",
	"Horrible",
	"Bad",
	"Poor",
	"Average",
	"Fair",
	"Good",
	"Excellent",
	"Awesome",
};

std::string getStatLevel(uint stat)
{
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

std::string getStatColor(uint stat)
{
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

// ----- CreaturePosition -----

std::string CreaturePosition::names[CreaturePosition::COUNT] = {
	"stand",
	"sit",
	"lay",
	"kneel",
};
std::string CreaturePosition::passive_verbs[CreaturePosition::COUNT] = {
	"stand up",
	"sit down",
	"lay down",
	"kneel",
};
std::string CreaturePosition::active_verbs[CreaturePosition::COUNT] = {
	"stands up",
	"sits down",
	"lays down",
	"kneels",
};
std::string CreaturePosition::states[CreaturePosition::COUNT] = {
	"standing",
	"sitting",
	"laying down",
	"kneeling",
};

CreaturePosition CreaturePosition::lookup(const std::string& name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (strEq(name, names[i]))
			return i;
	return STAND;
}

// ----- Creature -----

void Creature::saveData(File::Writer& writer)
{
	Entity::saveData(writer);

	if (dead)
		writer.attr("creature", "dead", "yes");

	writer.attr("creature", "position", position.getName());

	if (coins)
		writer.attr("creature", "coins", coins);

	writer.attr("creature", "hp", health.cur);

	if (equipment.right_held)
		equipment.right_held->save(writer, "creature", "equip_rhand");
	if (equipment.left_held)
		equipment.left_held->save(writer, "creature", "equip_lhand");
	if (equipment.body_worn)
		equipment.body_worn->save(writer, "creature", "equip_body");
	if (equipment.back_worn)
		equipment.back_worn->save(writer, "creature", "equip_back");
	if (equipment.waist_worn)
		equipment.waist_worn->save(writer, "creature", "equip_waist");
}

void Creature::saveHook(File::Writer& writer)
{
	Entity::saveHook(writer);
	Hooks::saveCreature(this, writer);
}

int Creature::loadNode(File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
	FO_PARENT(Entity)
	FO_ATTR("creature", "dead")
	dead = node.getBool();
	FO_ATTR("creature", "position")
	position = CreaturePosition::lookup(node.getString());
	FO_ATTR("creature", "coins")
	coins = node.getInt();
	FO_ATTR("creature", "hp")
	health.cur = node.getInt();

	FO_ENTITY("creature", "equip_rhand")
	if (OBJECT(entity) == NULL) throw File::Error("Equipment is not an Object");
	equipment.right_held = OBJECT(entity);
	equipment.right_held->setOwner(this);
	FO_ENTITY("creature", "equip_lhand")
	if (OBJECT(entity) == NULL) throw File::Error("Equipment is not an Object");
	equipment.left_held = OBJECT(entity);
	equipment.left_held->setOwner(this);
	FO_ENTITY("creature", "equip_body")
	if (OBJECT(entity) == NULL) throw File::Error("Equipment is not an Object");
	equipment.body_worn = OBJECT(entity);
	equipment.body_worn->setOwner(this);
	FO_ENTITY("creature", "equip_back")
	if (OBJECT(entity) == NULL) throw File::Error("Equipment is not an Object");
	equipment.back_worn = OBJECT(entity);
	equipment.back_worn->setOwner(this);
	FO_ENTITY("creature", "equip_waist")
	if (OBJECT(entity) == NULL) throw File::Error("Equipment is not an Object");
	equipment.waist_worn = OBJECT(entity);
	equipment.waist_worn->setOwner(this);
	FO_NODE_END
}

int Creature::loadFinish()
{
	recalc();

	return 0;
}

// Creature
Creature::Creature()
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

void Creature::setOwner(Entity* owner)
{
	// type check
	assert(ROOM(owner));

	// set owner
	Entity::setOwner(owner);
	location = (Room*)owner;
}

Entity* Creature::getOwner() const
{
	return location;
}

// add an action
void Creature::addAction(IAction* action)
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

IAction* Creature::getAction() const
{
	if (actions.empty())
		return NULL;
	else
		return actions.front();
}

void Creature::cancelAction()
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
uint Creature::getRoundTime() const
{
	// no actions?  no round time
	if (actions.empty())
		return 0;

	/*
	uint rounds = 0;

	for (ActionList::const_iterator i = actions.begin(); i != actions.end(); ++i)
		rounds += (*i)->getRounds();
	*/
	uint rounds = actions.front()->getRounds();

	if (rounds < round_time)
		return 0;
	return rounds - round_time;
}

bool Creature::checkAlive()
{
	if (isDead()) {
		*this << "You are only a ghost.\n";
		return false;
	}
	return true;
}

bool Creature::checkMove()
{
	// can't move if you're dead
	if (!checkAlive())
		return false;

	if (!canMove()) {
		*this << "You cannot move.\n";
		return false;
	}

	return true;
}

bool Creature::checkSee()
{
	if (!canSee()) {
		*this << "You cannot see.\n";
		return false;
	}
	return true;
}

bool Creature::checkRound()
{
	// round time?
	uint rounds = getRoundTime();

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
bool Creature::enter(Room *new_room, Portal *old_portal)
{
	assert(new_room != NULL);
	Room* old_room = getRoom();

	// already here
	if (new_room->creatures.has(this))
		return false;

	// entering portal
	Portal* enter_portal = NULL;
	if (old_portal && old_room)
		enter_portal = old_portal->getRelativePortal(old_room);

	// zones
	Zone* old_zone = NULL;
	if (old_room)
		old_zone = old_room->getZone();
	Zone* new_zone = new_room->getZone();

	// did we go thru an portal?
	if (old_portal) {
		// "You go..." message
		*this << StreamMacro(old_portal->getGo()).set("actor", this).set("portal", old_portal) << "\n";

		// "So-and-so leaves thru..." message
		if (old_room)
			*old_room << StreamIgnore(this) << StreamMacro(old_portal->getLeaves()).set("actor", this).set("portal", old_portal) << "\n";
	}

	// valid portal?
	if (enter_portal)
		*new_room << StreamMacro(enter_portal->getEnters()).set("actor", this).set("portal", enter_portal) << "\n";
	else
		*new_room << StreamName(this, INDEFINITE, true) << " arrives.\n";

	new_room->addCreature(this);

	if (old_room)
		Events::sendLeaveRoom(old_room, this, old_portal, new_room);
	Events::sendEnterRoom(new_room, this, enter_portal, old_room);
	if (old_room && old_zone != new_zone)
		Events::sendLeaveZone(old_room, this, new_zone);
	if (old_room && old_zone != new_zone)
		Events::sendEnterZone(old_room, this, old_zone);

	doLook();

	return true;
}

void Creature::heal(uint amount)
{
	bool was_dead = isDead();
	health.cur += amount;
	int max = getMaxHP();
	if (health.cur > max)
		health.cur = max;
	// have we been resurrected?
	if (health.cur > 0 && was_dead) {
		dead = false;
		// FIXME EVENT
	}
}

bool Creature::damage(uint amount, Creature *trigger)
{
	// already dead?  no reason to continue
	if (isDead())
		return false;
	// do damage and event
	health.cur -= amount;
	// FIXME EVENT
	// caused death?
	if (health.cur <= 0 && !isDead()) {
		dead = true;
		kill(trigger);
		return true;
	}
	// still alive
	return false;
}

uint Creature::giveCoins(uint amount)
{
	uint space = UINT_MAX - coins;
	if (space < amount)
		return coins += space;
	else
		return coins += amount;
}

uint Creature::takeCoins(uint amount)
{
	if (amount > coins)
		return coins = 0;
	else
		return coins -= amount;
}

void Creature::heartbeat()
{
	// pending actions?
	if (!actions.empty()) {
		// one round
		++round_time;

		// last round?
		bool done = false;
		if (round_time >= actions.front()->getRounds()) {
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
	if (!isDead() && (MUD::getRounds() % (50 - getEffectiveStat(CreatureStatID::FORTITUDE) / 5)) == 0) {
		heal(1);
	}

	// affects
	for (AffectStatusList::iterator i = affects.begin(); i != affects.end();) {
		(*i)->update(this);

		// affect expire?
		if ((*i)->getTimeLeft() == 0) {
			(*i)->remove(this);
			i = affects.erase(i);
		} else {
			++i;
		}
	}

	// update handler
	Hooks::creatureHeartbeat(this);
}

void Creature::activate()
{
	Entity::activate();

	Object* obj;
	for (int i = 0; (obj = getEquipAt(i)) != NULL; ++i)
		obj->activate();
}

void Creature::deactivate()
{
	Object* obj;
	for (int i = 0; (obj = getEquipAt(i)) != NULL; ++i)
		obj->deactivate();

	Entity::deactivate();
}

int Creature::macroProperty(const StreamControl& stream, const std::string& comm, const MacroList& argv) const
{
	// HE / SHE
	if (strEq(comm, "he")) {
		stream << getGender().getHeShe();
	}
	// HIM / HER
	else if (strEq(comm, "him")) {
		stream << getGender().getHimHer();
	}
	// HIS / HER
	else if (strEq(comm, "his")) {
		stream << getGender().getHisHer();
	}
	// HIS / HERS
	else if (strEq(comm, "hers")) {
		stream << getGender().getHisHers();
	}
	// MAN / WOMAN
	else if (strEq(comm, "man")) {
		stream << getGender().getManWoman();
	}
	// MALE / FEMALE
	else if (strEq(comm, "male")) {
		stream << getGender().getMaleFemale();
	}
	// ALIVE / DEAD
	else if (strEq(comm, "alive")) {
		if (isDead())
			stream << "dead";
		else
			stream << "alive";
	}
	// POSITION
	else if (strEq(comm, "position")) {
		stream << getPosition().getActiveVerb();
	}
	// default...
	else {
		return Entity::macroProperty(stream, comm, argv);
	}

	return 0;
}

// recalc stats
void Creature::recalcStats()
{
	for (int i = 0; i < CreatureStatID::COUNT; ++i)
		effective_stats[i] = getBaseStat(i);
}

// recalc max health
void Creature::recalcHealth()
{
	health.max = (10 + getStatModifier(CreatureStatID::FORTITUDE)) * 10;

	// cap HP
	if (health.cur > health.max)
		health.cur = health.max;
}

// recalculate various stuff
void Creature::recalc()
{
	recalcStats();
	recalcHealth();
}

void Creature::displayEquip(const StreamControl& stream) const
{
	// inventory variables
	uint loc = 0;
	Object* obj;
	Object* last = NULL;
	bool didshow = false;

	// worn items
	while ((obj = getWornAt(loc++)) != NULL) {
		// hidden?  skip
		if (obj->isHidden())
			continue;
		// we had one already?
		if (last) {
			// prefix
			if (didshow) {
				stream << ", ";
			} else {
				stream << StreamMacro("  {$self.He} is wearing ").set("self", this);
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
			stream << StreamMacro("  {$self.He} is wearing ").set("self", this);
			didshow = true;
		}
		// show it
		stream << StreamName(last, INDEFINITE) << ".";
	}

	// held items
	loc = 0;
	didshow = false;
	last = NULL;
	while ((obj = getHeldAt(loc++)) != NULL) {
		// hidden?  skip
		if (obj->isHidden())
			continue;
		// we had one already?
		if (last) {
			// prefix
			if (didshow) {
				stream << ", ";
			} else {
				stream << StreamMacro("  {$self.He} is holding ").set("self", this);
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
			stream << StreamMacro("  {$self.He} is holding ").set("self", this);
			didshow = true;
		}
		// show it
		stream << StreamName(last, INDEFINITE) << ".";
	}

	// dead or position
	if (isDead())
		stream << StreamMacro("  {$self.He} is laying on the ground, dead.").set("self", this);
	else if (getPosition() != CreaturePosition::STAND)
		stream << StreamMacro("  {$self.He} is {$self.position}.").set("self", this);

	// health
	if (!isDead() && getMaxHP() > 0) {
		if (getHP() * 100 / getMaxHP() <= 25)
			stream << StreamMacro("  {$self.He} appears severely wounded.").set("self", this);
		else if (getHP() * 100 / getMaxHP() <= 75)
			stream << StreamMacro("  {$self.He} appears wounded.").set("self", this);
		else
			stream << StreamMacro("  {$self.He} appears to be in good health.").set("self", this);
	}
}

void Creature::displayAffects(const StreamControl& stream) const
{
	bool found = false;
	for (AffectStatusList::const_iterator i = affects.begin(); i != affects.end(); ++i) {
		if (!(*i)->getTitle().empty()) {
			if (!found) {
				found = true;
				stream << "Active affects:\n";
			}
			stream << "  " << (*i)->getTitle() << " [" << (*i)->getType().getName() << "]\n";
		}
	}

	if (!found)
		stream << "There are no active affects.\n";
}

// stat modifier
int Creature::getStatModifier(CreatureStatID stat) const
{
	assert(stat);

	return (getEffectiveStat(stat) - 50) / 10;
}

// add an affect
int Creature::addAffect(CreatureAffectGroup* affect)
{
	if (affect->apply(this))
		return -1;

	affects.push_back(affect);
	return 0;
}

// events
void Creature::handleEvent(const Event& event)
{
	Entity::handleEvent(event);
}

void Creature::broadcastEvent(const Event& event)
{
	if (equipment.right_held)
		MEvent.resend(event, equipment.right_held);
	if (equipment.left_held)
		MEvent.resend(event, equipment.left_held);
	if (equipment.body_worn)
		MEvent.resend(event, equipment.body_worn);
	if (equipment.back_worn)
		MEvent.resend(event, equipment.back_worn);
	if (equipment.waist_worn)
		MEvent.resend(event, equipment.waist_worn);
}
