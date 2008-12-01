/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <climits>

#include <algorithm>

#include "mud/room.h"
#include "common/string.h"
#include "common/error.h"
#include "mud/color.h"
#include "mud/server.h"
#include "mud/player.h"
#include "mud/npc.h"
#include "mud/weather.h"
#include "common/streams.h"
#include "mud/macro.h"
#include "common/rand.h"
#include "mud/zone.h"
#include "mud/object.h"
#include "mud/creature.h"
#include "mud/portal.h"
#include "mud/hooks.h"
#include "mud/efactory.h"
#include "mud/shadow-object.h"
#include "mud/unique-object.h"

/* constructor */
Room::Room ()
{
	/* clear de values */
	zone = NULL;
	flags.outdoors = false;
	flags.safe = false;
	flags.noweather = false;
	coins = 0;
}

Room::~Room () {
}

int
Room::load_node (File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_ATTR("room", "id")
			set_id(node.get_string());
		FO_ATTR("room", "name")
			set_name(node.get_string());
		FO_ATTR("room", "desc")
			set_desc(node.get_string());
		FO_ATTR("room", "outdoors")
			flags.outdoors = node.get_bool();
		FO_ATTR("room", "safe")
			flags.safe = node.get_bool();
		FO_ATTR("room", "noweather")
			flags.noweather = node.get_bool();
		FO_ATTR("room", "coins")
			coins = node.get_int();
		FO_ENTITY("room", "child")
			if (NPC(entity)) {
				add_creature(NPC(entity));
			} else if (OBJECT(entity)) {
				add_object(OBJECT(entity));
			} else if (PORTAL(entity)) {
				Portal* portal = PORTAL(entity);

				// direction checking
				if (!portal->get_dir().valid())
					throw File::Error(S("Portal has no dir"));
				if (get_portal_by_dir(portal->get_dir()) != NULL)
					throw File::Error(S("Duplicate portal direction"));

				// add
				portal->parent_room = this;
				portals[portal->get_dir()] = portal;

				// activate if necessary
				if (is_active())
					portal->activate();
			} else {
				throw File::Error(S("Room child is not an Npc, Object, or Portal"));
			}
		FO_PARENT(Entity)
	FO_NODE_END
}

int
Room::load_finish ()
{
	return 0;
}

/* save the stupid thing */
void
Room::save_data (File::Writer& writer)
{
	writer.attr(S("room"), S("id"), id);

	if (!name.empty())
		writer.attr(S("room"), S("name"), name.get_name());

	if (!desc.empty())
		writer.attr(S("room"), S("desc"), desc);
	
	Entity::save_data(writer);

	if (flags.outdoors)
		writer.attr(S("room"), S("outdoors"), true);
	if (flags.safe)
		writer.attr(S("room"), S("safe"), true);
	if (flags.noweather)
		writer.attr(S("room"), S("noweather"), true);

	if (coins)
		writer.attr(S("room"), S("coins"), coins);

	for (std::map<PortalDir,Portal*>::const_iterator i = portals.begin(); i != portals.end(); ++i) {
		if (i->second->get_owner() == this)
			i->second->save(writer, S("room"), S("child"));
	}

	for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i)
		(*i)->save(writer, S("room"), S("child"));

	for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i) {
		if (NPC(*i))
			(*i)->save(writer, S("room"), S("child"));
	}
}

void
Room::save_hook (File::Writer& writer)
{
	Entity::save_hook(writer);
	Hooks::save_room(this, writer);
}

Portal *
Room::find_portal (const std::string& e_name, uint c, uint *matches)
{
	assert (c != 0);

	if (matches)
		*matches = 0;

	for (std::map<PortalDir,Portal*>::const_iterator i = portals.begin(); i != portals.end(); ++i) {
		if (i->second->name_match (e_name)) {
			if (matches)
				++ *matches;
			if ((-- c) == 0)
				return i->second;
		}
	}
	return NULL;
}

Portal *
Room::get_portal_by_dir (PortalDir dir)
{
	std::map<PortalDir,Portal*>::iterator i = portals.find(dir);
	if (i != portals.end())
		return i->second;
	else
		return NULL;
}

Portal*
Room::new_portal (PortalDir dir)
{
	Portal *portal = new Portal ();
	if (portal == NULL)
		return NULL;
	portal->set_dir(dir);
	portal->parent_room = this;
	portals[dir] = portal;
	if (is_active())
		portal->activate();
	return portal;
}

bool
Room::register_portal (Portal* portal)
{
	assert(portal != NULL);
	assert(portal->get_target() == get_id());

	std::map<PortalDir,Portal*>::iterator i = portals.find(portal->get_dir().get_opposite());
	if (i == portals.end()) {
		portals[portal->get_dir().get_opposite()] = portal;
		return true;
	} else if (i->second == portal)
		return true; // already registered
	else
		return false; // another portal is here
}

void
Room::unregister_portal (Portal* portal)
{
	assert(portal != NULL);
	assert(portal->get_target() == get_id());

	std::map<PortalDir,Portal*>::iterator i = portals.find(portal->get_dir().get_opposite());
	if (i != portals.end() && i->second == portal)
		portals.erase(i);
}

// coins
uint
Room::give_coins (uint amount)
{
	return coins += amount < (UINT_MAX - coins) ? amount : (UINT_MAX - coins);
}
uint
Room::take_coins (uint amount)
{
	return coins -= amount < coins ? amount : coins;
}

/* update: one game tick */
void Room::heartbeat()
{
	// call update hook
	Hooks::room_heartbeat(this);
}

void Room::activate()
{
	Entity::activate();

	for (std::map<PortalDir,Portal*>::const_iterator i = portals.begin(); i != portals.end(); ++i)
		if (i->second->get_room() == this)
			i->second->activate();

	for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i)
		(*i)->activate();

	for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i)
		(*i)->activate();
}

void Room::deactivate()
{
	for (std::map<PortalDir,Portal*>::const_iterator i = portals.begin(); i != portals.end(); ++i) {
		if (i->second->get_room() == this)
			i->second->deactivate();
	}

	for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i)
		(*i)->deactivate();

	for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i)
		(*i)->deactivate();

	Entity::deactivate ();
}

void
Room::set_owner (Entity* s_owner)
{
}

Entity*
Room::get_owner () const
{
	return NULL;
}

void
Room::owner_release (Entity* child)
{
	// Creature?
	Creature* ch = CHARACTER(child);
	if (ch != NULL) {
		creatures.remove(ch);
		return;
	}

	// Object?
	Object* obj = OBJECT(child);
	if (obj != NULL) {
		objects.remove(obj);
		return;
	}

	// Portal?
	Portal* portal = PORTAL(child);
	if (portal != NULL) {
		std::map<PortalDir,Portal*>::iterator i = portals.find(portal->get_dir());
		if (i != portals.end() && i->second == portal)
			portals.erase(i);
	}

	// something we don't support
	assert(false);
}

/* print out Room */
void
Room::show (const StreamControl& stream, Creature* viewer)
{
	// if there's a hook for this, don't do our version
	if (Hooks::show_room(this, viewer))
		return;

	// basic info
	stream << "[ " << StreamName(*this, NONE, true) << " ]\n";
	stream << CDESC "  " << StreamMacro(get_desc(), S("room"), this, S("actor"), viewer) << CNORMAL;

	// we're outdoors - do that stuff
	if (is_outdoors ()) {
		// show weather
		if (!flags.noweather)
			stream << "  " << MWeather.get_current_desc();
		// show time
		if (MTime.time.is_day ()) {
			if (!MTime.calendar.day_text.empty())
				stream << "  " << MTime.calendar.day_text[get_random(MTime.calendar.day_text.size())];
		} else {
			if (!MTime.calendar.night_text.empty())
				stream << "  " << MTime.calendar.night_text[get_random(MTime.calendar.night_text.size())];
		}
	}
	stream << "\n";

	// lists of stuffs
	std::vector<Entity*> ents;
	ents.reserve(10);

	// portals
	for (std::map<PortalDir,Portal*>::const_iterator i = portals.begin(); i != portals.end(); ++i) {
		// portal not hidden?
		if (!i->second->is_hidden() && !i->second->is_disabled())
			ents.push_back(i->second);
	}
	if (!ents.empty()) {
		stream << "Obvious exits are ";
		for (size_t i = 0; i < ents.size(); ++i) {
			if (i > 0) {
				if (ents.size() == 2)
					stream << " and ";
				else if (i == ents.size() - 1)
					stream << ", and ";
				else
					stream << ", ";
			}
			if (PORTAL(ents[i])->is_standard())
				stream << CEXIT << PORTAL(ents[i])->get_relative_dir(this).get_name() << CNORMAL;
			else
				stream << StreamName(ents[i], INDEFINITE) << '[' << PORTAL(ents[i])->get_relative_dir(this).get_abbr() << ']';
		}
		stream << ".\n";
	}
	ents.clear();

	// displaying creatures and objects
	int displayed = 0;
	Entity* last = NULL;

	// show players and NPCs
	if (!creatures.empty()) {
		// iterator
		for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i) {
			// not ourselves
			if ((Creature*)(*i) != viewer) {
				// have we a last entry?
				if (last) {
					// pre-text
					if (!displayed)
						stream << "You see ";
					else
						stream << ", ";

					// name
					stream << StreamName(*last, INDEFINITE);

					++displayed;
				}

				// last is this one now
				last = (*i);
			}
		}
	}

	// object list
	if (!objects.empty()) {
		// iterator
		for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i) {
			// no hidden?
			if (!(*i)->is_hidden ()) {
				// have we a last item?
				if (last) {
					// pre-text
					if (!displayed)
						stream << "You see ";
					else
						stream << ", ";

					stream << StreamName(*last, INDEFINITE);

					++displayed;
				}

				// last is us
				last = (*i);
			}
		}
	}

	// left over last?
	if (last) {
		// pre-text
		if (!displayed)
			stream << "You see ";
		else {
			if (coins)
				stream << ", ";
			else if (displayed >  1)
				stream << ", and ";
			else
				stream << " and ";
		}

		// show name
		stream << StreamName(*last, INDEFINITE);

		++displayed;
	}

	// coins?
	if (coins) {
		if (!displayed)
			stream << "You see ";
		else if (displayed > 1)
			stream << ", and ";
		else
			stream << " and ";

		if (coins == 1)
			stream << "one coin";
		else
			stream << coins << " coins";

		++displayed;
	}

	// finish up
	if (displayed)
		stream << ".\n";
}

/* print all portals */
void
Room::show_portals (const StreamControl& stream)
{
	for (std::map<PortalDir,Portal*>::const_iterator i = portals.begin(); i != portals.end(); ++i)
		stream << StreamName(*i->second) << " <" << i->second->get_target() << ">\n";
}

/* broadcast a message to the Room */
void
Room::put (const std::string& msg, size_t len, std::vector<Creature*>* ignore_list)
{
	// iterator
	for (EList<Creature>::iterator i = creatures.begin(); i != creatures.end(); ++i) {
		// skip ignored creatures
		if (ignore_list != NULL) {
			if (std::find(ignore_list->begin(), ignore_list->end(), (*i)) != ignore_list->end())
				continue;
		}
		// output
		(*i)->stream_put(msg.c_str(), len);
	}
}

/* find a Creature by name */
Creature *
Room::find_creature (const std::string& cname, uint c, uint *matches)
{
	assert (c != 0);
	
	return CHARACTER(creatures.match (cname, c, matches));
}

/* find an object by name */
Object *
Room::find_object (const std::string& oname, uint c, uint *matches)
{
	assert (c != 0);

	return OBJECT(objects.match (oname, c, matches));
}

void
Room::add_creature (Creature* creature)
{
	assert(creature != NULL);

	creature->set_owner(this);
	creatures.add (creature);
}

void
Room::add_object (Object* object)
{
	assert(object != NULL);

	object->set_owner(this);
	objects.add(object);
}

unsigned long
Room::count_players () const
{
	unsigned long count = 0;
	for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i)
		if (PLAYER(*i))
			++count;
	return count;
}

void
Room::handle_event (const Event& event)
{
	Entity::handle_event(event);
}

void
Room::broadcast_event (const Event& event)
{
	// propogate to objects
	for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i)
		MEvent.resend(event, *i);

	// propogate to creatures
	for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i)
		MEvent.resend(event, *i);

	// propogate to portals
	for (std::map<PortalDir,Portal*>::const_iterator i = portals.begin(); i != portals.end(); ++i)
		MEvent.resend(event, i->second);
}

// StreamSink for room buffering
class
RoomStreamSink : public IStreamSink {
	public:
	RoomStreamSink (class Room& s_room) : room(s_room), buffer(), ignores() {}

	virtual void stream_put (const char* text, size_t len) { buffer.write(text, len); }
	virtual void stream_ignore (class Creature* ch) { ignores.push_back(ch); }
	virtual void stream_end ();

	private:
	class Room& room;
	StringBuffer buffer;
	typedef std::vector<class Creature*> IgnoreList;
	IgnoreList ignores;
};

// flush room output
void
RoomStreamSink::stream_end () {
	// send output
	std::string text = buffer.str();
	if (!text.empty()) {
		if (ignores.empty())
			room.put(text, text.size());
		else
			room.put(text, text.size(), &ignores);
		buffer.clear();
	}
}

IStreamSink*
Room::get_stream () {
	return new RoomStreamSink(*this);
}

StreamControl::StreamControl (Room& rptr) : sink(new RoomStreamSink(rptr)) {}

BEGIN_EFACTORY(Room)
	return new Room();
END_EFACTORY
