/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#include "mud/room.h"
#include "common/string.h"
#include "common/error.h"
#include "mud/color.h"
#include "mud/server.h"
#include "mud/player.h"
#include "common/streams.h"
#include "mud/zone.h"
#include "mud/hooks.h"
#include "scriptix/function.h"

const String ExitDetail::names[] = {
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

const String ExitUsage::names[] = {
	"walk",
	"climb",
	"crawl",
};

const String ExitDir::names[] = {
	"none",
	"north",
	"east",
	"south",
	"west",
	"northwest",
	"northeast",
	"southeast",
	"southwest",
};
ExitDir::dir_t ExitDir::opposites[] = {
	ExitDir::NONE,
	ExitDir::SOUTH,
	ExitDir::WEST,
	ExitDir::NORTH,
	ExitDir::EAST,
	ExitDir::SOUTHEAST,
	ExitDir::SOUTHWEST,
	ExitDir::NORTHWEST,
	ExitDir::NORTHEAST,
};

// local tables
namespace {
	// When You go {the-exit}
	const String exit_go_table[ExitDetail::COUNT][ExitUsage::COUNT] = {
		{ "You head to {$exit.d}.", "You climb {$exit.d}.", "You crawl to {$exit.d}." },
		{ "You go in {$exit.d}.", "You climb in {$exit.d}.", "You crawl in {$exit.d}." },
		{ "You get on {$exit.d}.", "You climb on {$exit.d}.", "You crawl on {$exit.d}." },
		{ "You head over {$exit.d}.", "You climb over {$exit.d}.", "You crawl over {$exit.d}." },
		{ "You go under {$exit.d}.", "You climb beneath {$exit.d}.", "You crawl under {$exit.d}." },
		{ "You head across {$exit.d}.", "You climb across {$exit.d}.", "You crawl across {$exit.d}." },
		{ "You go out {$exit.d}.", "You climb out {$exit.d}.", "You crawl out {$exit.d}." },
		{ "You go up {$exit.d}.", "You climb up {$exit.d}.", "You crawl up {$exit.d}." },
		{ "You go down {$exit.d}.", "You climb down {$exit.d}.", "You crawl down {$exit.d}." },
		{ "You head through {$exit.d}.", "You climb through {$exit.d}.", "You crawl through {$exit.d}." },
	};
	// When {person} goes {the-exit}
	const String exit_leaves_table[ExitDetail::COUNT][ExitUsage::COUNT] = {
		{ "{$actor.I} heads to {$exit.d}.", "{$actor.I} climbs {$exit.d}.", "{$actor.I} crawls to {$exit.d}." },
		{ "{$actor.I} goes in {$exit.d}.", "{$actor.I} climbs in {$exit.d}.", "{$actor.I} crawls in {$exit.d}." },
		{ "{$actor.I} gets on {$exit.d}.", "{$actor.I} climbs on {$exit.d}.", "{$actor.I} crawls on {$exit.d}." },
		{ "{$actor.I} heads over {$exit.d}.", "{$actor.I} climbs over {$exit.d}.", "{$actor.I} crawls over {$exit.d}." },
		{ "{$actor.I} goes under {$exit.d}.", "{$actor.I} climbs beneath {$exit.d}.", "{$actor.I} crawls under {$exit.d}." },
		{ "{$actor.I} heads across {$exit.d}.", "{$actor.I} climbs across {$exit.d}.", "{$actor.I} crawls across {$exit.d}." },
		{ "{$actor.I} goes out {$exit.d}.", "{$actor.I} climbs out {$exit.d}.", "{$actor.I} crawls out {$exit.d}." },
		{ "{$actor.I} goes up {$exit.d}.", "{$actor.I} climbs up {$exit.d}.", "{$actor.I} crawls up {$exit.d}." },
		{ "{$actor.I} goes down {$exit.d}.", "{$actor.I} climbs down {$exit.d}.", "{$actor.I} crawls down {$exit.d}." },
		{ "{$actor.I} heads through {$exit.d}.", "{$actor.I} climbs through {$exit.d}.", "{$actor.I} crawls through {$exit.d}." },
	};
	// When {person} enters from {the-exit}
	const String exit_enters_table[ExitDetail::COUNT][ExitUsage::COUNT] = {
		{ "{$actor.I} arrives from {$exit.d}.", "{$actor.I} climbs in from {$exit.d}.", "{$actor.I} crawls in from {$exit.d}." },
		{ "{$actor.I} comes out from {$exit.d}.", "{$actor.I} climbs out from {$exit.d}.", "{$actor.I} crawls out from {$exit.d}." },
		{ "{$actor.I} gets off {$exit.d}.", "{$actor.I} climbs off {$exit.d}.", "{$actor.I} crawls off {$exit.d}." },
		{ "{$actor.I} arrives from over {$exit.d}.", "{$actor.I} climbs over from {$exit.d}.", "{$actor.I} crawls over from {$exit.d}." },
		{ "{$actor.I} comes from under {$exit.d}.", "{$actor.I} climbs from beneath {$exit.d}.", "{$actor.I} crawls from under {$exit.d}." },
		{ "{$actor.I} arrives from across {$exit.d}.", "{$actor.I} climbs from across {$exit.d}.", "{$actor.I} crawls from across {$exit.d}." },
		{ "{$actor.I} comes in from {$exit.d}.", "{$actor.I} climbs in from {$exit.d}.", "{$actor.I} crawls in from {$exit.d}." },
		{ "{$actor.I} comes down from {$exit.d}.", "{$actor.I} climbs down {$exit.d}.", "{$actor.I} crawls down {$exit.d}." },
		{ "{$actor.I} comes up {$exit.d}.", "{$actor.I} climbs up {$exit.d}.", "{$actor.I} crawls up {$exit.d}." },
		{ "{$actor.I} comes through {$exit.d}.", "{$actor.I} climbs through from {$exit.d}.", "{$actor.I} crawls from through {$exit.d}." },
	};
}

ExitDir
ExitDir::lookup (StringArg name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (names[i] == name)
			return i;
	return NONE;
}

ExitUsage
ExitUsage::lookup (StringArg name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (names[i] == name)
			return i;
	return WALK;
}

ExitDetail
ExitDetail::lookup (StringArg name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (names[i] == name)
			return i;
	return NONE;
}

SCRIPT_TYPE(RoomExit);
RoomExit::RoomExit() : Entity(AweMUD_RoomExitType), name(),
	target(), dir(), usage(), detail(), parent_room(NULL),
	flags(), on_use(NULL), on_use_source()
{}

EntityName
RoomExit::get_name () const
{
	// default name w/ direction
	if (name.empty())
		return EntityName(EntityArticleClass::UNIQUE, dir.get_name());
	else
		return name;
}

Room *
RoomExit::get_target_room () const
{
	if (target)
		return ZoneManager.get_room (target);
	else
		return NULL;
}

RoomExit *
RoomExit::get_target_exit () const
{
	Room *r = ZoneManager.get_room (target);
	if (r == NULL)
		return NULL;

	RoomExit *e = r->get_exit_by_dir (dir.get_opposite());

	return e;
}

bool
RoomExit::is_valid () const
{
	return target && ZoneManager.get_room (target);
}

void
RoomExit::save (File::Writer& writer)
{
	if (!name.empty())
		writer.attr("name", name.get_name());

	if (!desc.empty())
		writer.attr("desc", desc);
	
	Entity::save (writer);

	for (StringList::const_iterator i = keywords.begin(); i != keywords.end(); ++i)
		writer.attr("keyword", *i);

	if (dir.valid())
		writer.attr ("dir", dir.get_name());
	if (usage != ExitUsage::WALK)
		writer.attr ("usage", usage.get_name());
	if (detail != ExitDetail::NONE)
		writer.attr ("detail", detail.get_name());
	if (is_hidden ())
		writer.attr ("hidden", "yes");
	if (is_door ()) {
		writer.attr ("door", "yes");
		if (is_closed ())
			writer.attr ("closed", "yes");
		if (is_locked ())
			writer.attr ("locked", "yes");
		if (!is_synced ())
			writer.attr ("nosync", "yes");
	}
	if (is_nolook())
		writer.attr ("nolook", "yes");
	if (is_disabled())
		writer.attr ("disabled", "yes");

	if (!target.empty())
		writer.attr("target", target);

	if (text.enters)
		writer.attr ("enters", text.enters);
	if (text.leaves)
		writer.attr ("leaves", text.leaves);
	if (text.go)
		writer.attr ("go", text.go);

	if (on_use_source)
		writer.block ("used", on_use_source);
}

void
RoomExit::save_hook (ScriptRestrictedWriter* writer)
{
	Entity::save_hook(writer);
	Hooks::save_exit(this, writer);
}

int
RoomExit::load_node (File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_ATTR("name")
			set_name(node.get_data());
		FO_ATTR("keyword")
			keywords.push_back(node.get_data());
		FO_ATTR("desc")
			set_desc(node.get_data());
		FO_ATTR("usage")
			usage = ExitUsage::lookup(node.get_data());
		FO_ATTR("dir")
			dir = ExitDir::lookup(node.get_data());
		FO_ATTR("direction") // duplicate of above - should we keep this?
			dir = ExitDir::lookup(node.get_data());
		FO_ATTR("detail")
			detail = ExitDetail::lookup(node.get_data());
		FO_ATTR("hidden")
			FO_TYPE_ASSERT(BOOL);
			set_hidden(str_is_true(node.get_data()));
		FO_ATTR("door")
			FO_TYPE_ASSERT(BOOL);
			set_door(str_is_true(node.get_data()));
		FO_ATTR("closed")
			FO_TYPE_ASSERT(BOOL);
			set_closed(str_is_true(node.get_data()));
		FO_ATTR("locked")
			FO_TYPE_ASSERT(BOOL);
			set_locked(str_is_true(node.get_data()));
		FO_ATTR("nosync")
			FO_TYPE_ASSERT(BOOL);
			set_synced(str_is_true(node.get_data()));
		FO_ATTR("nolook")
			FO_TYPE_ASSERT(BOOL);
			set_nolook(str_is_true(node.get_data()));
		FO_ATTR("disabled")
			FO_TYPE_ASSERT(BOOL);
			set_disabled(str_is_true(node.get_data()));
		FO_ATTR("target")
			target = node.get_data();
		FO_ATTR("enters")
			text.enters = node.get_data();
		FO_ATTR("leaves")
			text.leaves = node.get_data();
		FO_ATTR("go")
			text.go = node.get_data();
		FO_ATTR("used")
			on_use_source = node.get_data();
			on_use = Scriptix::ScriptFunction::compile("used", on_use_source, "exit,user", reader.get_filename(), node.get_line());
		FO_PARENT(Entity)
	FO_NODE_END
}

int
RoomExit::load_finish ()
{
	return 0;
}

void
RoomExit::open () {
	flags.closed = false;;

	if (is_synced ()) {
		RoomExit *other = get_target_exit ();
		if (other) {
			other->flags.closed = false;;
			*other->get_room() << StreamName(other, DEFINITE, true) << " is opened from the other side.\n";
		}
	}
}

void
RoomExit::close () {
	flags.closed = true;;

	if (is_synced ()) {
		RoomExit *other = get_target_exit ();
		if (other) {
			other->flags.closed = true;;
			*other->get_room() << StreamName(other, DEFINITE, true) << " is closed from the other side.\n";
		}
	}
}

void
RoomExit::unlock () {
	flags.locked = false;;

	if (is_synced ()) {
		RoomExit *other = get_target_exit ();
		if (other) {
			other->flags.locked = false;;
			*other->get_room() << "A click eminates from " << StreamName(other) << ".\n";
		}
	}
}

void
RoomExit::lock () {
	flags.locked = true;;

	if (is_synced ()) {
		RoomExit *other = get_target_exit ();
		if (other) {
			other->flags.locked = true;;
			*other->get_room() << "A click eminates from " << StreamName(other) << ".\n";
		}
	}
}

void
RoomExit::heartbeat ()
{
}

void
RoomExit::set_owner (Entity* owner)
{
	assert(ROOM(owner));
	Entity::set_owner(owner);
	parent_room = (Room*)owner;
}

Entity*
RoomExit::get_owner () const
{
	return parent_room;
}

void
RoomExit::owner_release (Entity* child)
{
	// we have no children
	assert(false);
}

StringArg
RoomExit::get_go () const
{
	// customized?
	if (text.go)
		return text.go;

	// use table
	return exit_go_table[detail.get_value()][usage.get_value()];
}

StringArg
RoomExit::get_leaves () const
{
	// customized?
	if (text.leaves)
		return text.leaves;

	// use table
	return exit_leaves_table[detail.get_value()][usage.get_value()];
}

StringArg
RoomExit::get_enters () const
{
	// customized?
	if (text.enters)
		return text.enters;

	// use table
	return exit_enters_table[detail.get_value()][usage.get_value()];
}

bool
RoomExit::operator< (const RoomExit& exit) const
{
	// empty names always first
	if (name.empty() && !exit.name.empty())
		return true;
	else if (!name.empty() && exit.name.empty())
		return false;
		
	// sort by direction
	if (dir.get_value() < exit.dir.get_value())
		return true;
	else if (dir.get_value() > exit.dir.get_value())
		return false;

	// then name
	return get_name() < exit.get_name();
}

bool
RoomExit::name_match (StringArg match) const
{
	if (name.matches(match))
		return true;

	// try keywords
	for (StringList::const_iterator i = keywords.begin(); i != keywords.end(); i ++)
		if (phrase_match (*i, match))
			return true;

	// no match
	return false;
}
