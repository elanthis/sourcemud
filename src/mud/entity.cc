/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/error.h"
#include "common/string.h"
#include "common/streams.h"
#include "mud/entity.h"
#include "mud/server.h"
#include "mud/macro.h"
#include "mud/color.h"
#include "mud/player.h"
#include "mud/clock.h"
#include "mud/hooks.h"

// ----- Entity -----

Entity::Entity() : state(FLOAT)
{
	// add to the dead list; we move to the live list
	// only if we get activated
	link_prev = NULL;
	link_next = MEntity.dead;
	if (link_next)
		link_next->link_prev = this;
	MEntity.dead = this;

	// create Lua object
	Lua::createObject(this, "Entity");
}

Entity::~Entity()
{
	// release our Lua object
	Lua::releaseObject(this);
}

EventHandler* Entity::get_event(EventID name)
{
	for (EventList::iterator i = events.begin(); i != events.end(); ++i) {
		if ((*i)->get_event() == name)
			return *i;
	}
	return NULL;
}

void Entity::activate()
{
	// must be in FLOAT state
	assert(state == FLOAT && "state must be FLOAT to activate");

	// set as active
	state = ACTIVE;

	// assign unique ID if it has none
	if (!uid)
		uid = MUniqueID.create();

	// insert into unique ID table
	MEntity.id_map.insert(std::pair<UniqueID, Entity*>(uid, this));

	// remove from dead list
	if (link_next)
		link_next->link_prev = link_prev;
	if (link_prev)
		link_prev->link_next = link_next;
	if (this == MEntity.dead)
		MEntity.dead = link_next;

	// add to active list
	link_prev = NULL;
	link_next = MEntity.all;
	if (link_next)
		link_next->link_prev = this;
	MEntity.all = this;

	// register tags
	for (TagList::iterator i = tags.begin(); i != tags.end(); ++i)
		MEntity.tag_map.insert(std::pair<TagID, Entity*> (*i, this));
}

void Entity::deactivate()
{
	// must be active
	assert(state == ACTIVE && "state must be ACTIVE to deactivate");

	// quite dead, thank you
	state = DEAD;

	// ID MAP
	UniqueIDMap::iterator i = MEntity.id_map.find(uid);
	if (i != MEntity.id_map.end())
		MEntity.id_map.erase(i);

	// if we're the next heartbeat target, update the heartbeat pointer
	if (this == MEntity.next)
		MEntity.next = link_next;

	// remove from active list
	if (link_next)
		link_next->link_prev = link_prev;
	if (link_prev)
		link_prev->link_next = link_next;
	if (this == MEntity.all)
		MEntity.all = link_next;

	// add to dead list
	link_prev = NULL;
	link_next = MEntity.dead;
	if (link_next)
		link_next->link_prev = this;
	MEntity.dead = this;

	// TAG MAP
	for (TagList::iterator i = tags.begin(); i != tags.end(); ++i) {
		std::pair<TagTable::iterator, TagTable::iterator> mi = MEntity.tag_map.equal_range(*i);
		while (mi.first != mi.second) {
			if (mi.first->second == this)
				MEntity.tag_map.erase(mi.first++);
			else
				++mi.first;
		}
	}
}

void Entity::destroy()
{
	Entity* owner = get_owner();
	if (owner != NULL)
		owner->owner_release(this);
	if (is_active())
		deactivate();
}

bool Entity::name_match(const std::string& match) const
{
	return get_name().matches(match);

	// no match
	return false;
}

void Entity::display_desc(const StreamControl& stream) const
{
	stream << StreamMacro(get_desc(), "self", this);
}

void Entity::save(File::Writer& writer, const std::string& ns, const std::string& name)
{
	writer.begin_attr(ns, name, factory_type());
	save_data(writer);
	writer.end();
}

void Entity::save_data(File::Writer& writer)
{
	writer.attr("entity", "uid", uid);

	// event handler list
	for (EventList::const_iterator i = events.begin(); i != events.end(); i ++) {
		writer.begin("entity", "event");
		(*i)->save(writer);
		writer.end();
	}

	// save tags
	for (TagList::const_iterator i = tags.begin(); i != tags.end(); ++i)
		writer.attr("entity", "tag", TagID::nameof(*i));

	// call save hook
	save_hook(writer);
}

void Entity::save_hook(File::Writer& writer)
{
	Hooks::save_entity(this, writer);
}

// load
Entity* Entity::load(const std::string& factory, File::Reader& reader)
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

int Entity::load(File::Reader& reader)
{
	// load the thing
	FO_READ_BEGIN
}
else if (load_node(reader, node) == FO_SUCCESS_CODE)
{
	FO_READ_ERROR
	return -1;
	FO_READ_END

	// final check
	if (load_finish() != 0)
		return -1;

	return 0;
}

int Entity::load_node(File::Reader& reader, File::Node& node)
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

int Entity::macro_property(const StreamControl& stream, const std::string& comm, const MacroList& argv) const
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
	if (str_eq(comm, "name")) {
		stream << StreamName(this);
		return 0;
		// ENTITY'S DESC
	} else if (str_eq(comm, "desc")) {
		display_desc(stream);
		return 0;
	}

	return -1;
}

void Entity::macro_default(const StreamControl& stream) const
{
	stream << StreamName(*this);
}

bool Entity::has_tag(TagID tag) const
{
	return tags.find(tag) != tags.end();
}

int Entity::add_tag(TagID tag)
{
	// no duplicates
	if (has_tag(tag))
		return 1;

	// add tag
	tags.insert(tag);

	// register with entity manager
	// FIXME: check for error, maybe?
	if (is_active())
		MEntity.tag_map.insert(std::pair<TagID, Entity*> (tag, this));

	return 0;
}

int Entity::remove_tag(TagID tag)
{
	// find
	TagList::iterator ti = std::find(tags.begin(), tags.end(), tag);
	if (ti == tags.end())
		return 1;

	// remove
	tags.erase(ti);

	// unregister with entity manager
	if (is_active()) {
		std::pair<TagTable::iterator, TagTable::iterator> mi = MEntity.tag_map.equal_range(tag);
		while (mi.first != mi.second) {
			if (mi.first->second == this) {
				MEntity.tag_map.erase(mi.first);
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

void Entity::set_owner(Entity* owner)
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

// ----- _MEntity -----

_MEntity MEntity;

_MEntity::_MEntity()
{
}

_MEntity::~_MEntity()
{
}

int _MEntity::initialize()
{
	return 0; // no error
}

void _MEntity::shutdown()
{
	tag_map.clear();
	collect();
}

void _MEntity::heartbeat()
{
	// loop over all entities, running the heartbeat method.
	// keep track of the next entity to be run, so that
	// entity destruction won't screw us up.  also note that
	// entity creation won't screw us up, because entities
	// are always added to the front of the list.
	Entity* next = NULL;
	Entity* cur = all;
	while (cur != NULL) {
		next = cur->link_next;
		cur->heartbeat();
		cur = next;
	}
}

size_t _MEntity::tag_count(TagID tag) const
{
	return tag_map.count(tag);
}

std::pair<TagTable::const_iterator, TagTable::const_iterator> _MEntity::tag_list(TagID tag) const
{
	return tag_map.equal_range(tag);
}

Entity* _MEntity::get(const UniqueID& uid) const
{
	UniqueIDMap::const_iterator i = id_map.find(uid);
	if (i != id_map.end())
		return i->second;
	return NULL;
}

void _MEntity::collect()
{
	// delete all dead entities
	// we remove the entity from the list first, then we
	// delete it, just incase the destructor kills some
	// more entities; that shouldn't happen, but better
	// safe than sorry.
	while (dead != NULL) {
		Entity* e = dead;
		dead = dead->link_next;
		if (dead != NULL)
			dead->link_prev = NULL;

		delete e;
	}
}
