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

EntityName Portal::get_name() const
{
	// default name w/ direction
	if (name.empty())
		return EntityName(EntityArticleClass::UNIQUE, dir.get_name());
	else
		return name;
}

void Portal::add_keyword(const std::string& keyword)
{
	keywords.push_back(keyword);
}

Room* Portal::get_relative_target(Room* base) const
{
	assert(base != NULL);

	// if we're asking about the owner, return the 'target' room
	if (base == parent_room)
		return MZone.get_room(target);
	// if we're the target room, return the owner
	else if (base->get_id() == target)
		return parent_room;
	// otherwise, we're not involved with this portal at all
	else
		return NULL;
}

Portal* Portal::get_relative_portal(Room* base) const
{
	assert(base != NULL);

	// if we're a two-way portal, it's always ourself
	if (!is_oneway())
		return const_cast<Portal*>(this);

	// if we're the portal's owner, get the target's opposite portal
	if (base == parent_room) {
		Room* room = MZone.get_room(target);
		if (room == NULL)
			return NULL;
		return room->get_portal_by_dir(dir.get_opposite());
	}

	// we're a one-way portal and not the owner, so go away
	return NULL;
}

PortalDir Portal::get_relative_dir(Room* base) const
{
	assert(base != NULL);

	// owner uses the base dir
	if (base == parent_room)
		return dir;
	// target uses the opposite dir
	else if (base->get_id() == target)
		return dir.get_opposite();
	// we're not related to this room
	else
		return PortalDir();
}

bool Portal::has_room(Room* base) const
{
	assert(base != NULL);

	return (base == parent_room || base->get_id() == target);
}

bool Portal::is_valid() const
{
	return !target.empty() && MZone.get_room(target);
}

void Portal::save_data(File::Writer& writer)
{
	if (!name.empty())
		writer.attr("portal", "name", name.get_name());

	if (!desc.empty())
		writer.attr("portal", "desc", desc);

	Entity::save_data(writer);

	for (std::vector<std::string>::const_iterator i = keywords.begin(); i != keywords.end(); ++i)
		writer.attr("portal", "keyword", *i);

	if (dir.valid())
		writer.attr("portal", "dir", dir.get_name());
	if (usage != PortalUsage::WALK)
		writer.attr("portal", "usage", usage.get_name());
	if (detail != PortalDetail::NONE)
		writer.attr("portal", "detail", detail.get_name());
	if (is_hidden())
		writer.attr("portal", "hidden", true);
	if (is_door()) {
		writer.attr("portal", "door", true);
		if (is_closed())
			writer.attr("portal", "closed", true);
		if (is_locked())
			writer.attr("portal", "locked", true);
	}
	if (is_nolook())
		writer.attr("portal", "nolook", true);
	if (is_disabled())
		writer.attr("portal", "disabled", true);
	if (is_oneway())
		writer.attr("portal", "oneway", true);

	if (!target.empty())
		writer.attr("portal", "target", target);
}

void Portal::save_hook(File::Writer& writer)
{
	Entity::save_hook(writer);
	Hooks::save_portal(this, writer);
}

int Portal::load_node(File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
	FO_ATTR("portal", "name")
	set_name(node.get_string());
	FO_ATTR("portal", "keyword")
	keywords.push_back(node.get_string());
	FO_ATTR("portal", "desc")
	set_desc(node.get_string());
	FO_ATTR("portal", "usage")
	usage = PortalUsage::lookup(node.get_string());
	FO_ATTR("portal", "dir")
	dir = PortalDir::lookup(node.get_string());
	FO_ATTR("portal", "direction") // duplicate of above - should we keep this?
	dir = PortalDir::lookup(node.get_string());
	FO_ATTR("portal", "detail")
	detail = PortalDetail::lookup(node.get_string());
	FO_ATTR("portal", "hidden")
	set_hidden(node.get_bool());
	FO_ATTR("portal", "door")
	set_door(node.get_bool());
	FO_ATTR("portal", "closed")
	set_closed(node.get_bool());
	FO_ATTR("portal", "locked")
	set_locked(node.get_bool());
	FO_ATTR("portal", "oneway")
	set_oneway(node.get_bool());
	FO_ATTR("portal", "nolook")
	set_nolook(node.get_bool());
	FO_ATTR("portal", "disabled")
	set_disabled(node.get_bool());
	FO_ATTR("portal", "target")
	target = node.get_string();
	FO_PARENT(Entity)
	FO_NODE_END
}

int Portal::load_finish()
{
	return 0;
}

void Portal::open(Room* base, Creature* actor)
{
	flags.closed = false;

	if (!is_oneway()) {
		Room* other = get_relative_target(base);
		if (other)
			*other << StreamName(other, DEFINITE, true) << " is opened from the other side by " << StreamName(actor, INDEFINITE, false) << ".\n";
	}
}

void Portal::close(Room* base, Creature* actor)
{
	flags.closed = true;;

	if (!is_oneway()) {
		Room* other = get_relative_target(base);
		if (other)
			*other << StreamName(other, DEFINITE, true) << " is closed from the other side by " << StreamName(actor, INDEFINITE, false) << ".\n";
	}
}

void Portal::unlock(Room* base, Creature* actor)
{
	flags.locked = false;

	if (!is_oneway()) {
		Room* other = get_relative_target(base);
		if (other)
			*other << "A click eminates from " << StreamName(other, DEFINITE) << ".\n";
	}
}

void Portal::lock(Room* base, Creature* actor)
{
	flags.locked = true;;

	if (!is_oneway()) {
		Room* other = get_relative_target(base);
		if (other)
			*other << "A click eminates from " << StreamName(other, DEFINITE) << ".\n";
	}
}

void Portal::heartbeat()
{
}

void Portal::set_owner(Entity* owner)
{
	assert(ROOM(owner));
	Entity::set_owner(owner);
	parent_room = (Room*)owner;
}

Entity* Portal::get_owner() const
{
	return parent_room;
}

void Portal::owner_release(Entity* child)
{
	// we have no children
	assert(false);
}

std::string Portal::get_go() const
{
	// use table
	return portal_go_table[detail.get_value()][usage.get_value()];
}

std::string Portal::get_leaves() const
{
	// use table
	return portal_leaves_table[detail.get_value()][usage.get_value()];
}

std::string Portal::get_enters() const
{
	// use table
	return portal_enters_table[detail.get_value()][usage.get_value()];
}

bool Portal::name_match(const std::string& match) const
{
	if (name.matches(match))
		return true;

	// try keywords
	for (std::vector<std::string>::const_iterator i = keywords.begin(); i != keywords.end(); i ++)
		if (phrase_match(*i, match))
			return true;

	// no match
	return false;
}

void Portal::activate()
{
	Entity::activate();

	if (!is_oneway()) {
		Room* room = MZone.get_room(target);
		if (room != NULL) {
			if (!room->register_portal(this)) {
				Log::Warning << "Room '" << room->get_id() << "' already has portal for direction " << get_dir().get_opposite().get_name() << ", converting " << get_dir().get_name() << " portal of room '" << parent_room->get_id() << "' to one-way";
				set_oneway(true);
			}
		}
	}
}

void Portal::deactivate()
{
	if (!is_oneway()) {
		Room* room = MZone.get_room(target);
		if (room != NULL)
			room->unregister_portal(this);
	}

	Entity::deactivate();
}

void Portal::handle_event(const Event& event)
{
	Entity::handle_event(event);
}

void Portal::broadcast_event(const Event& event)
{
}

BEGIN_EFACTORY(Portal)
return new Portal();
END_EFACTORY
