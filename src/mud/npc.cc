/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/file.h"
#include "common/streams.h"
#include "common/string.h"
#include "mud/body.h"
#include "mud/npc.h"
#include "mud/room.h"
#include "mud/macro.h"
#include "mud/settings.h"
#include "mud/object.h"
#include "mud/skill.h"
#include "mud/hooks.h"
#include "mud/efactory.h"

Npc::Npc() : Creature()
{
	initialize();
}

Npc::Npc(NpcBP* s_blueprint) : Creature()
{
	initialize();
	blueprint = NULL;
	setBlueprint(s_blueprint);
}

void Npc::initialize()
{
	blueprint = NULL;
	flags.zonelock = false;
	flags.revroomtag = false;
	room_tag = TagID();
}

Npc::~Npc()
{
}

int Npc::loadFinish()
{
	if (Creature::loadFinish())
		return -1;

	if (blueprint == NULL) {
		Log::Error << "NPC has no blueprint";
		return -1;
	}

	return 0;
}

int Npc::loadNode(File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
	FO_ATTR("npc", "blueprint")
	NpcBP* blueprint;
	if ((blueprint = MNpcBP.lookup(node.getString())) == NULL)
		Log::Error << "Could not find npc blueprint '" << node.getString() << "'";
	else
		setBlueprint(blueprint);
	FO_ATTR("npc", "roomtag")
	room_tag = TagID::create(node.getString());
	FO_ATTR("npc", "zonelock")
	flags.zonelock = node.getBool();
	FO_ATTR("npc", "reverse_roomtag")
	flags.revroomtag = node.getBool();
	FO_PARENT(Creature)
	FO_NODE_END
}

void Npc::saveData(File::Writer& writer)
{
	if (getBlueprint())
		writer.attr("npc", "blueprint", getBlueprint()->getId());

	Creature::saveData(writer);

	if (room_tag.valid())
		writer.attr("npc", "roomtag", TagID::nameof(room_tag));
	if (flags.zonelock)
		writer.attr("npc", "zonelock", true);
	if (flags.revroomtag)
		writer.attr("npc", "reverse_roomtag", true);
}

void Npc::saveHook(File::Writer& writer)
{
	Creature::saveHook(writer);
	Hooks::saveNpc(this, writer);
}

EntityName Npc::getName() const
{
	assert(blueprint != NULL);
	return blueprint->getName();
}

std::string Npc::getDesc() const
{
	assert(blueprint != NULL);
	return blueprint->getDesc();
}

GenderType Npc::getGender() const
{
	assert(blueprint != NULL);
	return blueprint->getGender();
}

int Npc::getBaseStat(CreatureStatID stat) const
{
	assert(blueprint != NULL);
	return blueprint->getStat(stat);
}

uint Npc::getCombatDodge() const
{
	assert(blueprint != NULL);
	return blueprint->getCombatDodge();
}

uint Npc::getCombatAttack() const
{
	assert(blueprint != NULL);
	return blueprint->getCombatAttack();
}

uint Npc::getCombatDamage() const
{
	assert(blueprint != NULL);
	return blueprint->getCombatDamage();
}

void Npc::kill(Creature *killer)
{
	// death message
	if (getRoom())
		*getRoom() << StreamName(this, DEFINITE, true) << " has been slain!\n";

	// lay down, drop crap
	position = CreaturePosition::LAY;

	// FIXME EVENT
	if (!Hooks::npcDeath(this, killer)) {
		// only if there's no hook - hooks must do this for us!
		destroy();
	}
}

void Npc::handleEvent(const Event& event)
{
	// normal event handler
	Entity::handleEvent(event);
}

void Npc::heartbeat()
{
	// do creature update
	Creature::heartbeat();

	// update handler
	Hooks::npcHeartbeat(this);
}

void Npc::setBlueprint(NpcBP* s_blueprint)
{
	blueprint = s_blueprint;
	for (int i = 0; i < CreatureStatID::COUNT; ++i)
		Creature::setEffectiveStat(CreatureStatID(i), getBaseStat(CreatureStatID(i)));
	Creature::recalc();
}

// load npc from a blueprint
Npc* Npc::loadBlueprint(const std::string& name)
{
	// lookup the blueprint
	NpcBP* blueprint = MNpcBP.lookup(name);
	if (!blueprint)
		return NULL;

	// create it
	Npc* npc = new Npc(blueprint);
	if (npc == NULL)
		return NULL;

	// equip
	while (blueprint != NULL) {
		for (std::vector<std::string>::const_iterator i = blueprint->getEquipList().begin(); i != blueprint->getEquipList().end(); ++i) {
			Object* object = Object::loadBlueprint(*i);
			if (object != NULL)
				npc->equip(object);
			else
				Log::Error << "Object blueprint '" << *i << "' from NPC blueprint '" << blueprint->getId() << "' does not exist.";
		}
		blueprint = blueprint->getParent();
	}

	// set HP
	npc->setHP(npc->getMaxHP());

	return npc;
}

// display NPC description
void Npc::displayDesc(const StreamControl& stream)
{
	if (!getDesc().empty())
		stream << StreamMacro(getDesc()).set("npc", this); // FIXME: re-enable 'actor'(looker)
	else
		stream << StreamName(this, DEFINITE, true) << " doesn't appear very interesting.";
}

bool Npc::canUsePortal(Portal* portal) const
{
	assert(portal != NULL);

	// disabled portals can't be used
	if (portal->isDisabled())
		return false;

	// portal's room is our room, yes?
	if (!portal->hasRoom(getRoom()))
		return false;

	// get target room; must be valid
	Room* troom = portal->getRelativeTarget(getRoom());
	if (troom == NULL)
		return false;

	// check zone constraints
	if (isZoneLocked() && troom->getZone() != getRoom()->getZone())
		return false;

	// check room tag constraints
	if (room_tag.valid()) {
		// does the room have the tag?
		if (troom->hasTag(room_tag)) {
			// reversed?
			if (isRoomTagReversed())
				return false;
			// does not have tag
		} else {
			// required it?
			if (!isRoomTagReversed())
				return false;
		}
	}

	// FIXME: check portal usage types; climb, crawl, etc

	// no failures - all good
	return true;
}

bool Npc::isBlueprint(const std::string& name) const
{
	NpcBP* blueprint = getBlueprint();

	while (blueprint != NULL) {
		if (strEq(blueprint->getId(), name))
			return true;

		blueprint = blueprint->getParent();
	}

	return false;
}

bool Npc::nameMatch(const std::string& match) const
{
	if (getName().matches(match))
		return true;

	// blueprint keywords
	NpcBP* blueprint = getBlueprint();
	while (blueprint != NULL) {
		for (std::vector<std::string>::const_iterator i = blueprint->getKeywords().begin(); i != blueprint->getKeywords().end(); i ++)
			if (phraseMatch(*i, match))
				return true;

		blueprint = blueprint->getParent();
	}

	// no match
	return false;
}

BEGIN_EFACTORY(NPC)
	return new Npc();
END_EFACTORY
