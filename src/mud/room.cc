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
#include "mud/parse.h"
#include "common/rand.h"
#include "mud/zone.h"
#include "mud/object.h"
#include "mud/creature.h"
#include "mud/portal.h"
#include "mud/hooks.h"
#include "mud/efactory.h"

/* constructor */
SCRIPT_TYPE(Room);
Room::Room () : Entity (AweMUD_RoomType)
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
			set_id(node.get_data());
		FO_ATTR("room", "name")
			set_name(node.get_data());
		FO_ATTR("room", "desc")
			set_desc(node.get_data());
		FO_ATTR("room", "outdoors")
			FO_TYPE_ASSERT(BOOL);
			flags.outdoors = str_is_true(node.get_data());
		FO_ATTR("room", "safe")
			FO_TYPE_ASSERT(BOOL);
			flags.safe = str_is_true(node.get_data());
		FO_ATTR("room", "noweather")
			FO_TYPE_ASSERT(BOOL);
			flags.noweather = str_is_true(node.get_data());
		FO_ATTR("room", "coins")
			FO_TYPE_ASSERT(INT);
			coins = tolong(node.get_data());
		FO_OBJECT("portal")
			Portal* portal = new Portal();
			if (portal == NULL)
				throw File::Error(S("new Portal() failed"));
			if (portal->load(reader))
				throw File::Error(S("Failed to load portal"));

			// add
			portal->parent_room = this;
			portals.add(portal);

			// activate if necessary
			if (is_active())
				portal->activate();
		FO_OBJECT("object")
			Object *obj = new Object ();
			if (obj->load (reader) != 0)
				throw File::Error(S("Failed to load object"));
			add_object(obj);
		FO_OBJECT("npc")
			Npc *npc = new Npc ();
			if (npc->load (reader) != 0)
				throw File::Error(S("Failed to load npc"));
			add_creature(npc);
		FO_PARENT(Entity)
	FO_NODE_END
}

int
Room::load_finish ()
{
	// ensure portals are sorted
	sort_portals();
	
	return 0;
}

/* save the stupid thing */
void
Room::save (File::Writer& writer)
{
	writer.attr(S("room"), S("id"), id);

	if (!name.empty())
		writer.attr(S("room"), S("name"), name.get_name());

	if (!desc.empty())
		writer.attr(S("room"), S("desc"), desc);
	
	Entity::save(writer);

	if (flags.outdoors)
		writer.attr(S("room"), S("outdoors"), true);
	if (flags.safe)
		writer.attr(S("room"), S("safe"), true);
	if (flags.noweather)
		writer.attr(S("room"), S("noweather"), true);

	if (coins)
		writer.attr(S("room"), S("coins"), coins);

	for (EList<Portal>::const_iterator i = portals.begin(); i != portals.end(); ++i) {
		writer.begin(S("portal"));
		(*i)->save(writer);
		writer.end();
	}

	for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i) {
		writer.begin(S("object"));
		(*i)->save (writer);
		writer.end();
	}

	for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i) {
		if (NPC(*i)) {
			writer.begin(S("npc"));
			(*i)->save(writer);
			writer.end();
		}
	}
}

void
Room::save_hook (ScriptRestrictedWriter* writer)
{
	Entity::save_hook(writer);
	Hooks::save_room(this, writer);
}

Portal *
Room::find_portal (String e_name, uint c, uint *matches)
{
	assert (c != 0);

	if (matches)
		*matches = 0;

	for (EList<Portal>::const_iterator i = portals.begin(); i != portals.end(); ++i) {
		if ((*i)->name_match (e_name)) {
			if (matches)
				++ *matches;
			if ((-- c) == 0)
				return (*i);
		}
	}
	return NULL;
}

Portal *
Room::get_portal_by_dir (PortalDir dir)
{
	for (EList<Portal>::const_iterator i = portals.begin(); i != portals.end(); ++i)
		if ((*i)->get_dir () == dir)
			return (*i);
	return NULL;
}

Portal *
Room::new_portal ()
{
	Portal *portal = new Portal ();
	if (portal == NULL)
		return NULL;
	portal->parent_room = this;
	portal->activate();
	portals.add(portal);
	return portal;
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
void
Room::heartbeat ()
{
	// call update hook
	Hooks::room_heartbeat(this);
}

void
Room::activate ()
{
	Entity::activate ();

	for (EList<Portal>::const_iterator i = portals.begin(); i != portals.end(); ++i)
		(*i)->activate();
	for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i)
		(*i)->activate();
	for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i)
		(*i)->activate();
}

void
Room::deactivate ()
{
	for (EList<Portal>::const_iterator i = portals.begin(); i != portals.end(); ++i)
		(*i)->deactivate();
	for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i)
		(*i)->deactivate();
	for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i)
		(*i)->deactivate();

	Entity::deactivate ();
}

void
Room::set_owner (Entity* s_owner)
{
	assert(ZONE(s_owner));
	Entity::set_owner(s_owner);
	zone = (Zone*)s_owner;
}

Entity*
Room::get_owner () const
{
	return zone;
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
		portals.remove(portal);
		return;
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
	stream << CDESC "  " << StreamParse(get_desc(), S("room"), this, S("actor"), viewer) << CNORMAL;


	// we're outdoors - do that stuff
	if (is_outdoors ()) {
		// show weather
		if (!flags.noweather)
			stream << "  " << WeatherManager.get_current_desc();
		// show time
		if (TimeManager.time.is_day ()) {
			if (!TimeManager.calendar.day_text.empty())
				stream << "  " << TimeManager.calendar.day_text[get_random(TimeManager.calendar.day_text.size())];
		} else {
			if (!TimeManager.calendar.night_text.empty())
				stream << "  " << TimeManager.calendar.night_text[get_random(TimeManager.calendar.night_text.size())];
		}
	}
	stream << "\n";

	// portal list
	if (!portals.empty()) {
		// setup
		int displayed = 0;
		Portal* last = NULL;
		
		// iterator
		for (EList<Portal>::const_iterator i = portals.begin(); i != portals.end(); ++i) {
			// room not hidden?
			if (!(*i)->is_hidden() && !(*i)->is_disabled()) {
				// had a previous entry?  output it
				if (last) {
					// first item?
					if (!displayed)
						stream << "Obvious exits are ";
					else
						stream << ", ";

					// print name
					if (last->is_closed())
						stream << "[";
					stream << StreamName(*last, NONE);
					if (last->is_closed())
						stream << "]";

					++displayed;
				}

				// last item was this item
				last = *i;
			}
		}
		
		// one left over?
		if (last) {
			// pre-text
			if (!displayed)
				stream << "Obvious exits are ";
			else if (displayed > 1)
				stream << ", and ";
			else
				stream << " and ";

			// show name
			stream << StreamName(*last, NONE);

			++displayed;
		}

		// finish up if we printed anything
		if (displayed)
			stream << ".\n";
	}

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
	for (EList<Portal>::const_iterator i = portals.begin(); i != portals.end(); ++i)
		stream << StreamName(*(*i)) << " <" << (*i)->get_target() << ">\n";
}

/* broadcast a message to the Room */
void
Room::put (String msg, size_t len, GCType::vector<Creature*>* ignore_list)
{
	// iterator
	for (EList<Creature>::iterator i = creatures.begin(); i != creatures.end(); ++i) {
		// skip ignored creatures
		if (ignore_list != NULL) {
			if (std::find(ignore_list->begin(), ignore_list->end(), (*i)) != ignore_list->end())
				continue;
		}
		// output
		(*i)->stream_put(msg, len);
	}
}

/* find a Creature by name */
Creature *
Room::find_creature (String cname, uint c, uint *matches)
{
	assert (c != 0);
	
	return CHARACTER(creatures.match (cname, c, matches));
}

/* find an object by name */
Object *
Room::find_object (String oname, uint c, uint *matches)
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

	// initialize NPC AI for new NPCs
	if (NPC(creature)) {
		if (((Npc*)creature)->get_ai())
			((Npc*)creature)->get_ai()->do_load(creature);
	}
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

int
Room::handle_event (const Event& event)
{
	// handle in self first
	Entity::handle_event (event);

	// temporary, stable vector of children
	EList<Entity> children(objects.size() + creatures.size() + portals.size());
	size_t index = 0;

	// propogate to objects
	for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i)
		children[index++] = *i;

	// propogate to creatures
	for (EList<Creature>::const_iterator i = creatures.begin(); i != creatures.end(); ++i)
		children[index++] = *i;

	// propogate to portals
	for (EList<Portal>::const_iterator i = portals.begin(); i != portals.end(); ++i)
		children[index++] = *i;

	// do event sending
	for (EList<Entity>::const_iterator i = children.begin(); i != children.end(); ++i)
		(*i)->handle_event(event);

	return 0;
}

// custom dereferencing sorting operator...
namespace {
	template <typename T>
	class DerefSort
	{
		public:
		bool operator()(const T* f1, const T* f2) {
			return *f1 < *f2;
		}
	};
}

void
Room::sort_portals ()
{
	std::sort(portals.begin(), portals.end(), DerefSort<Portal>());
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
	typedef GCType::vector<class Creature*> IgnoreList;
	IgnoreList ignores;
};

// flush room output
void
RoomStreamSink::stream_end () {
	// send output
	String text = buffer.str();
	if (text) {
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
