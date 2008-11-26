/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */


#include <stdlib.h>
#include <stdio.h>
#include <typeinfo>
#include <algorithm>

#include "mud/entity.h"
#include "common/error.h"
#include "common/string.h"
#include "mud/server.h"
#include "mud/macro.h"
#include "common/streams.h"
#include "mud/color.h"
#include "mud/player.h"
#include "mud/clock.h"
#include "mud/hooks.h"

// ----- Entity -----

Entity::Entity() : state(FLOAT) {}

EventHandler*
Entity::get_event (EventID name)
{
	for (EventList::iterator i = events.begin(); i != events.end(); ++i) {
		if ((*i)->get_event() == name)
			return *i;
	}
	return NULL;
}

void
Entity::activate (void)
{
	// must be in FLOAT state
	assert(state == FLOAT && "state must be FLOAT to activate");

	// assign unique ID if it has none
	if (!uid)
		uid = UniqueIDManager.create();

	// insert into unique ID table
	EntityManager.id_map.insert(std::pair<UniqueID, Entity*>(uid, this));

	// find best heartbeat
	eheartbeat = 0;
	for (uint8 i = 1; i < TICKS_PER_ROUND; ++i)
		if (EntityManager.elist[i].size() < EntityManager.elist[eheartbeat].size())
			eheartbeat = i;

	// add to manager's big list
	EntityManager.elist[eheartbeat].push_front(this);

	// register tags
	for (TagList::iterator i = tags.begin(); i != tags.end(); ++i) 
		EntityManager.tag_map.insert(std::pair<TagID, Entity*> (*i, this));

	state = ACTIVE;
}

void
Entity::deactivate (void)
{
	// must be active
	assert(state == ACTIVE && "state must be ACTIVE to deactivate");

	// ID MAP
	UniqueIDMap::iterator i = EntityManager.id_map.find(uid);
	if (i != EntityManager.id_map.end())
		EntityManager.id_map.erase(i);

	// BIG LIST
	EntityManager.elist[eheartbeat].erase(std::find(EntityManager.elist[eheartbeat].begin(), EntityManager.elist[eheartbeat].end(), this));

	// TAG MAP
	for (TagList::iterator i = tags.begin(); i != tags.end(); ++i) {
		std::pair<TagTable::iterator, TagTable::iterator> mi = EntityManager.tag_map.equal_range(*i);
		while (mi.first != mi.second) {
			if (mi.first->second == this)
				EntityManager.tag_map.erase(mi.first++);
			else
				++mi.first;
		}
	}

	// store for deletion
	EntityManager.dead.push_front(this);

	state = DEAD;
}

void
Entity::destroy (void)
{
	Entity* owner = get_owner();
	if (owner != NULL)
		owner->owner_release(this);
	if (is_active())
		deactivate();
}

bool
Entity::name_match (String match) const
{
	return get_name().matches(match);

	// no match
	return false;
}

void
Entity::display_desc (const StreamControl& stream) const
{
	stream << StreamMacro(get_desc(), S("self"), this);
}

void
Entity::save (File::Writer& writer, String ns, String name)
{
	writer.begin_attr(ns, name, factory_type());
	save_data(writer);
	writer.end();
}

void
Entity::save_data (File::Writer& writer)
{
	writer.attr(S("entity"), S("uid"), uid);

	// event handler list
	for (EventList::const_iterator i = events.begin (); i != events.end (); i ++) {
		writer.begin(S("entity"), S("event"));
		(*i)->save(writer);
		writer.end();
	}

	// save tags
	for (TagList::const_iterator i = tags.begin(); i != tags.end(); ++i)
		writer.attr(S("entity"), S("tag"), TagID::nameof(*i));

	// call save hook
	save_hook(writer);
}

void
Entity::save_hook (File::Writer& writer)
{
	Hooks::save_entity(this, writer);
}

// load
Entity*
Entity::load (String factory, File::Reader& reader)
{
	Entity* entity;

	// attempt to create entity
	entity = create(factory);
	if (entity == NULL) {
		Log::Error << "Entity factory '" << factory << "' failed";
		return NULL;
	}

	// attempt load
	if (entity->load(reader) != 0)
		return NULL;
	
	// ok
	return entity;
}

int
Entity::load (File::Reader& reader)
{
	// load the thing
	FO_READ_BEGIN
		} else if (load_node(reader, node) == FO_SUCCESS_CODE) {
	FO_READ_ERROR
		return -1;
	FO_READ_END

	// final check
	if (load_finish() != 0)
		return -1;

	return 0;
}

int
Entity::load_node (File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_ATTR("entity", "uid")
			uid = node.get_id();
		FO_ATTR("entity", "tag")
			add_tag(TagID::create(node.get_string()));
		FO_OBJECT("entity", "event")
			EventHandler* event = new EventHandler();
			if (!event->load(reader))
					events.push_back(event);
	FO_NODE_END
}

int
Entity::macro_property (const StreamControl& stream, String comm, const MacroList& argv) const
{
	// SPECIAL: one-letter name commands
	if (comm.size() == 1) {
		switch (comm[0]) {
			case 'D':
				stream << StreamName(this, DEFINITE, true);
				return 0;
			case 'd':
				stream << StreamName(this, DEFINITE, false);
				return 0;
			case 'C':
			case 'I':
				stream << StreamName(this, INDEFINITE, true);
				return 0;
			case 'c':
			case 'i':
				stream << StreamName(this, INDEFINITE, false);
				return 0;
			case 'N':
				stream << StreamName(this, NONE, true);
				return 0;
			case 'n':
				stream << StreamName(this, NONE, false);
				return 0;
			default:
				return -1;
		}
	}

	// ENTITY's NAME
	if (str_eq(comm, S("name"))) {
		stream << StreamName(this);
		return 0;
	// ENTITY'S DESC
	} else if (str_eq(comm, S("desc"))) {
		display_desc(stream);
		return 0;
	}

	return -1;
}

void
Entity::macro_default (const StreamControl& stream) const
{
	stream << StreamName(*this);
}

bool
Entity::has_tag (TagID tag) const
{
	return tags.find(tag) != tags.end();
}

int
Entity::add_tag (TagID tag)
{
	// no duplicates
	if (has_tag(tag))
		return 1;

	// add tag
	tags.insert(tag);

	// register with entity manager
	// FIXME: check for error, maybe?
	if (is_active())
		EntityManager.tag_map.insert(std::pair<TagID, Entity*> (tag, this));

	return 0;
}

int
Entity::remove_tag (TagID tag)
{
	// find
	TagList::iterator ti = std::find(tags.begin(), tags.end(), tag);
	if (ti == tags.end())
		return 1;

	// remove
	tags.erase(ti);

	// unregister with entity manager
	if (is_active()) {
		std::pair<TagTable::iterator, TagTable::iterator> mi = EntityManager.tag_map.equal_range(tag);
		while (mi.first != mi.second) {
			if (mi.first->second == this) {
				EntityManager.tag_map.erase(mi.first);
				return 0;
			}
			++mi.first;
		}
		return 2; // failed to find in manager
	} else {
		// no active - no need to unregister
		return 0;
	}
}

void
Entity::set_owner (Entity* owner)
{
	assert(owner != NULL);

	Entity* old_owner = get_owner();
	if (old_owner != NULL)
		old_owner->owner_release(this);

	if (is_active() && !owner->is_active())
		deactivate();
	else if (!is_active() && owner->is_active())
		activate();
}

bool
Entity::operator< (const Entity& ent) const
{
	return get_name() < ent.get_name();
}

// ----- SEntityManager -----

SEntityManager EntityManager;

SEntityManager::SEntityManager (void) : elist(), ecur(), eheartbeat(0)
{
}

SEntityManager::~SEntityManager (void)
{
}

int
SEntityManager::initialize (void)
{
	return 0; // no error
}

void
SEntityManager::shutdown (void)
{
	for (uint8 i = 0; i < TICKS_PER_ROUND; ++i)
		elist[i].clear();
	tag_map.clear();
	collect();
}

void
SEntityManager::heartbeat (void)
{
	// loop
	ecur = elist[eheartbeat].begin();
	Entity* cur;
	while (ecur != elist[eheartbeat].end()) {
		// get current
		cur = *ecur;

		// move ptr
		++ecur;

		// update entity
		cur->heartbeat();
	}

	// increment heartbeat
	if (++eheartbeat == TICKS_PER_ROUND)
		eheartbeat = 0;
}

size_t
SEntityManager::tag_count (TagID tag) const
{
	return tag_map.count(tag);
}

std::pair<TagTable::const_iterator, TagTable::const_iterator>
SEntityManager::tag_list (TagID tag) const
{
	return tag_map.equal_range(tag);
}

Entity*
SEntityManager::get (const UniqueID& uid) const
{
	UniqueIDMap::const_iterator i = id_map.find(uid);
	if (i != id_map.end())
		return i->second;
	return NULL;
}

void SEntityManager::collect()
{
	while (!dead.empty()) {
		Entity* e = dead.front();
		dead.erase(dead.begin());
		delete e;
	}
}
