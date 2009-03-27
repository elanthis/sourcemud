/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/string.h"
#include "common/error.h"
#include "common/streams.h"
#include "mud/room.h"
#include "mud/color.h"
#include "mud/server.h"
#include "mud/player.h"
#include "mud/zone.h"
#include "mud/hooks.h"
#include "mud/efactory.h"

const std::string PortalDetail::names[] = {
	"none",
	"in",
	"on",
	"over",
	"under",
	"across",
	"out",
	"up",
	"down",
	"through",
};

const std::string PortalUsage::names[] = {
	"walk",
	"climb",
	"crawl",
};

const std::string PortalDir::names[] = {
	"none",
	"north",
	"east",
	"south",
	"west",
	"northwest",
	"northeast",
	"southeast",
	"southwest",
	"up",
	"down",
};
const std::string PortalDir::abbreviations[] = {
	"x",
	"n",
	"e",
	"s",
	"w",
	"nw",
	"ne",
	"se",
	"sw",
	"u",
	"d",
};
PortalDir::dir_t PortalDir::opposites[] = {
	PortalDir::NONE,
	PortalDir::SOUTH,
	PortalDir::WEST,
	PortalDir::NORTH,
	PortalDir::EAST,
	PortalDir::SOUTHEAST,
	PortalDir::SOUTHWEST,
	PortalDir::NORTHWEST,
	PortalDir::NORTHEAST,
	PortalDir::DOWN,
	PortalDir::UP,
};

// local tables
namespace
{
	// When You go {the-portal}
	const std::string portal_go_table[PortalDetail::COUNT][PortalUsage::COUNT] = {
		{
			"You head to {$portal.d}.",
			"You climb {$portal.d}.",
			"You crawl to {$portal.d}."
		}, {
			"You go in {$portal.d}.",
			"You climb in {$portal.d}.",
			"You crawl in {$portal.d}."
		}, {
			"You get on {$portal.d}.",
			"You climb on {$portal.d}.",
			"You crawl on {$portal.d}."
		}, {
			"You head over {$portal.d}.",
			"You climb over {$portal.d}.",
			"You crawl over {$portal.d}."
		}, {
			"You go under {$portal.d}.",
			"You climb beneath {$portal.d}.",
			"You crawl under {$portal.d}."
		}, {
			"You head across {$portal.d}.",
			"You climb across {$portal.d}.",
			"You crawl across {$portal.d}."
		}, {
			"You go out {$portal.d}.",
			"You climb out {$portal.d}.",
			"You crawl out {$portal.d}."
		}, {
			"You go up {$portal.d}.",
			"You climb up {$portal.d}.",
			"You crawl up {$portal.d}."
		}, {
			"You go down {$portal.d}.",
			"You climb down {$portal.d}.",
			"You crawl down {$portal.d}."
		}, {
			"You head through {$portal.d}.",
			"You climb through {$portal.d}.",
			"You crawl through {$portal.d}."
		},
	};
	// When {person} goes {the-portal}
	const std::string portal_leaves_table[PortalDetail::COUNT][PortalUsage::COUNT] = {
		{
			"{$actor.I} heads to {$portal.d}.",
			"{$actor.I} climbs {$portal.d}.",
			"{$actor.I} crawls to {$portal.d}."
		}, {
			"{$actor.I} goes in {$portal.d}.",
			"{$actor.I} climbs in {$portal.d}.",
			"{$actor.I} crawls in {$portal.d}."
		}, {
			"{$actor.I} gets on {$portal.d}.",
			"{$actor.I} climbs on {$portal.d}.",
			"{$actor.I} crawls on {$portal.d}."
		}, {
			"{$actor.I} heads over {$portal.d}.",
			"{$actor.I} climbs over {$portal.d}.",
			"{$actor.I} crawls over {$portal.d}."
		}, {
			"{$actor.I} goes under {$portal.d}.",
			"{$actor.I} climbs beneath {$portal.d}.",
			"{$actor.I} crawls under {$portal.d}."
		}, {
			"{$actor.I} heads across {$portal.d}.",
			"{$actor.I} climbs across {$portal.d}.",
			"{$actor.I} crawls across {$portal.d}."
		}, {
			"{$actor.I} goes out {$portal.d}.",
			"{$actor.I} climbs out {$portal.d}.",
			"{$actor.I} crawls out {$portal.d}."
		}, {
			"{$actor.I} goes up {$portal.d}.",
			"{$actor.I} climbs up {$portal.d}.",
			"{$actor.I} crawls up {$portal.d}."
		}, {
			"{$actor.I} goes down {$portal.d}.",
			"{$actor.I} climbs down {$portal.d}.",
			"{$actor.I} crawls down {$portal.d}."
		}, {
			"{$actor.I} heads through {$portal.d}.",
			"{$actor.I} climbs through {$portal.d}.",
			"{$actor.I} crawls through {$portal.d}."
		},
	};
	// When {person} enters from {the-portal}
	const std::string portal_enters_table[PortalDetail::COUNT][PortalUsage::COUNT] = {
		{
			"{$actor.I} arrives from {$portal.d}.",
			"{$actor.I} climbs in from {$portal.d}.",
			"{$actor.I} crawls in from {$portal.d}."
		}, {
			"{$actor.I} comes out from {$portal.d}.",
			"{$actor.I} climbs out from {$portal.d}.",
			"{$actor.I} crawls out from {$portal.d}."
		}, {
			"{$actor.I} gets off {$portal.d}.",
			"{$actor.I} climbs off {$portal.d}.",
			"{$actor.I} crawls off {$portal.d}."
		}, {
			"{$actor.I} arrives from over {$portal.d}.",
			"{$actor.I} climbs over from {$portal.d}.",
			"{$actor.I} crawls over from {$portal.d}."
		}, {
			"{$actor.I} comes from under {$portal.d}.",
			"{$actor.I} climbs from beneath {$portal.d}.",
			"{$actor.I} crawls from under {$portal.d}."
		}, {
			"{$actor.I} arrives from across {$portal.d}.",
			"{$actor.I} climbs from across {$portal.d}.",
			"{$actor.I} crawls from across {$portal.d}."
		}, {
			"{$actor.I} comes in from {$portal.d}.",
			"{$actor.I} climbs in from {$portal.d}.",
			"{$actor.I} crawls in from {$portal.d}."
		}, {
			"{$actor.I} comes down from {$portal.d}.",
			"{$actor.I} climbs down {$portal.d}.",
			"{$actor.I} crawls down {$portal.d}."
		}, {
			"{$actor.I} comes up {$portal.d}.",
			"{$actor.I} climbs up {$portal.d}.",
			"{$actor.I} crawls up {$portal.d}."
		}, {
			"{$actor.I} comes through {$portal.d}.",
			"{$actor.I} climbs through from {$portal.d}.",
			"{$actor.I} crawls from through {$portal.d}."
		},
	};
}

PortalDir PortalDir::lookup(const std::string& name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (names[i] == name)
			return i;
	return NONE;
}

PortalUsage PortalUsage::lookup(const std::string& name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (names[i] == name)
			return i;
	return WALK;
}

PortalDetail PortalDetail::lookup(const std::string& name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (names[i] == name)
			return i;
	return NONE;
}

Portal::Portal() : parent_room(NULL)
{}

EntityName Portal::getName() const
{
	// default name w/ direction
	if (name.empty())
		return EntityName(EntityArticleClass::UNIQUE, dir.getName());
	else
		return name;
}

void Portal::addKeyword(const std::string& keyword)
{
	keywords.push_back(keyword);
}

Room* Portal::getRelativeTarget(Room* base) const
{
	assert(base != NULL);

	// if we're asking about the owner, return the 'target' room
	if (base == parent_room)
		return MZone.getRoom(target);
	// if we're the target room, return the owner
	else if (base->getId() == target)
		return parent_room;
	// otherwise, we're not involved with this portal at all
	else
		return NULL;
}

Portal* Portal::getRelativePortal(Room* base) const
{
	assert(base != NULL);

	// if we're a two-way portal, it's always ourself
	if (!isOneway())
		return const_cast<Portal*>(this);

	// if we're the portal's owner, get the target's opposite portal
	if (base == parent_room) {
		Room* room = MZone.getRoom(target);
		if (room == NULL)
			return NULL;
		return room->getPortalByDir(dir.getOpposite());
	}

	// we're a one-way portal and not the owner, so go away
	return NULL;
}

PortalDir Portal::getRelativeDir(Room* base) const
{
	assert(base != NULL);

	// owner uses the base dir
	if (base == parent_room)
		return dir;
	// target uses the opposite dir
	else if (base->getId() == target)
		return dir.getOpposite();
	// we're not related to this room
	else
		return PortalDir();
}

bool Portal::hasRoom(Room* base) const
{
	assert(base != NULL);

	return (base == parent_room || base->getId() == target);
}

bool Portal::isValid() const
{
	return !target.empty() && MZone.getRoom(target);
}

void Portal::saveData(File::Writer& writer)
{
	if (!name.empty())
		writer.attr("portal", "name", name.getFull());

	if (!desc.empty())
		writer.attr("portal", "desc", desc);

	Entity::saveData(writer);

	for (std::vector<std::string>::const_iterator i = keywords.begin(); i != keywords.end(); ++i)
		writer.attr("portal", "keyword", *i);

	if (dir.valid())
		writer.attr("portal", "dir", dir.getName());
	if (usage != PortalUsage::WALK)
		writer.attr("portal", "usage", usage.getName());
	if (detail != PortalDetail::NONE)
		writer.attr("portal", "detail", detail.getName());
	if (isHidden())
		writer.attr("portal", "hidden", true);
	if (isDoor()) {
		writer.attr("portal", "door", true);
		if (isClosed())
			writer.attr("portal", "closed", true);
		if (isLocked())
			writer.attr("portal", "locked", true);
	}
	if (isNolook())
		writer.attr("portal", "nolook", true);
	if (isDisabled())
		writer.attr("portal", "disabled", true);
	if (isOneway())
		writer.attr("portal", "oneway", true);

	if (!target.empty())
		writer.attr("portal", "target", target);
}

void Portal::saveHook(File::Writer& writer)
{
	Entity::saveHook(writer);
	Hooks::savePortal(this, writer);
}

int Portal::loadNode(File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
	FO_ATTR("portal", "name")
	setName(node.getString());
	FO_ATTR("portal", "keyword")
	keywords.push_back(node.getString());
	FO_ATTR("portal", "desc")
	setDesc(node.getString());
	FO_ATTR("portal", "usage")
	usage = PortalUsage::lookup(node.getString());
	FO_ATTR("portal", "dir")
	dir = PortalDir::lookup(node.getString());
	FO_ATTR("portal", "direction") // duplicate of above - should we keep this?
	dir = PortalDir::lookup(node.getString());
	FO_ATTR("portal", "detail")
	detail = PortalDetail::lookup(node.getString());
	FO_ATTR("portal", "hidden")
	setHidden(node.getBool());
	FO_ATTR("portal", "door")
	setDoor(node.getBool());
	FO_ATTR("portal", "closed")
	setClosed(node.getBool());
	FO_ATTR("portal", "locked")
	setLocked(node.getBool());
	FO_ATTR("portal", "oneway")
	setOneway(node.getBool());
	FO_ATTR("portal", "nolook")
	setNolook(node.getBool());
	FO_ATTR("portal", "disabled")
	setDisabled(node.getBool());
	FO_ATTR("portal", "target")
	target = node.getString();
	FO_PARENT(Entity)
	FO_NODE_END
}

int Portal::loadFinish()
{
	return 0;
}

void Portal::open(Room* base, Creature* actor)
{
	flags.closed = false;

	if (!isOneway()) {
		Room* other = getRelativeTarget(base);
		if (other)
			*other << StreamName(other, DEFINITE, true) << " is opened from the other side by " << StreamName(actor, INDEFINITE, false) << ".\n";
	}
}

void Portal::close(Room* base, Creature* actor)
{
	flags.closed = true;;

	if (!isOneway()) {
		Room* other = getRelativeTarget(base);
		if (other)
			*other << StreamName(other, DEFINITE, true) << " is closed from the other side by " << StreamName(actor, INDEFINITE, false) << ".\n";
	}
}

void Portal::unlock(Room* base, Creature* actor)
{
	flags.locked = false;

	if (!isOneway()) {
		Room* other = getRelativeTarget(base);
		if (other)
			*other << "A click eminates from " << StreamName(other, DEFINITE) << ".\n";
	}
}

void Portal::lock(Room* base, Creature* actor)
{
	flags.locked = true;;

	if (!isOneway()) {
		Room* other = getRelativeTarget(base);
		if (other)
			*other << "A click eminates from " << StreamName(other, DEFINITE) << ".\n";
	}
}

void Portal::heartbeat()
{
}

void Portal::setOwner(Entity* owner)
{
	assert(ROOM(owner));
	Entity::setOwner(owner);
	parent_room = (Room*)owner;
}

Entity* Portal::getOwner() const
{
	return parent_room;
}

void Portal::ownerRelease(Entity* child)
{
	// we have no children
	assert(false);
}

std::string Portal::getGo() const
{
	// use table
	return portal_go_table[detail.getValue()][usage.getValue()];
}

std::string Portal::getLeaves() const
{
	// use table
	return portal_leaves_table[detail.getValue()][usage.getValue()];
}

std::string Portal::getEnters() const
{
	// use table
	return portal_enters_table[detail.getValue()][usage.getValue()];
}

bool Portal::nameMatch(const std::string& match) const
{
	if (name.matches(match))
		return true;

	// try keywords
	for (std::vector<std::string>::const_iterator i = keywords.begin(); i != keywords.end(); i ++)
		if (phraseMatch(*i, match))
			return true;

	// no match
	return false;
}

void Portal::activate()
{
	Entity::activate();

	if (!isOneway()) {
		Room* room = MZone.getRoom(target);
		if (room != NULL) {
			if (!room->registerPortal(this)) {
				Log::Warning << "Room '" << room->getId() << "' already has portal for direction " << getDir().getOpposite().getName() << ", converting " << getDir().getName() << " portal of room '" << parent_room->getId() << "' to one-way";
				setOneway(true);
			}
		}
	}
}

void Portal::deactivate()
{
	if (!isOneway()) {
		Room* room = MZone.getRoom(target);
		if (room != NULL)
			room->unregisterPortal(this);
	}

	Entity::deactivate();
}

void Portal::handleEvent(const Event& event)
{
	Entity::handleEvent(event);
}

void Portal::broadcastEvent(const Event& event)
{
}

BEGIN_EFACTORY(Portal)
return new Portal();
END_EFACTORY
