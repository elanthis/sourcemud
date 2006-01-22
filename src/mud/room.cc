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
#include "mud/char.h"
#include "mud/exit.h"
#include "mud/hooks.h"

/* constructor */
SCRIPT_TYPE(Room);
Room::Room (void) : Entity (AweMUD_RoomType)
{
	/* clear de values */
	zone = NULL;
	flags.outdoors = false;
	flags.safe = false;
	flags.noweather = false;
	coins = 0;
}

Room::~Room (void) {
}

int
Room::load_node (File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_ATTR("id")
			set_id(node.get_data());
		FO_ATTR("name")
			set_name(node.get_data());
		FO_ATTR("desc")
			set_desc(node.get_data());
		FO_ATTR("outdoors")
			FO_TYPE_ASSERT(BOOL);
			flags.outdoors = str_is_true(node.get_data());
		FO_ATTR("safe")
			FO_TYPE_ASSERT(BOOL);
			flags.safe = str_is_true(node.get_data());
		FO_ATTR("noweather")
			FO_TYPE_ASSERT(BOOL);
			flags.noweather = str_is_true(node.get_data());
		FO_ATTR("coins")
			FO_TYPE_ASSERT(INT);
			coins = tolong(node.get_data());
		FO_OBJECT("exit")
			RoomExit* exit = new RoomExit();
			if (exit == NULL)
				throw File::Error("new RoomExit() failed");
			if (exit->load(reader))
				throw File::Error("failed to load exit");

			// add
			exit->parent_room = this;
			exits.add(exit);

			// activate if necessary
			if (is_active())
				exit->activate();
		FO_OBJECT("object")
			Object *obj = new Object ();
			if (obj->load (reader) != 0)
				throw File::Error("failed to load object");
			add_object(obj);
		FO_OBJECT("npc")
			Npc *npc = new Npc ();
			if (npc->load (reader) != 0)
				throw File::Error("failed to load npc");
			add_character(npc);
		FO_PARENT(Entity)
	FO_NODE_END
}

int
Room::load_finish (void)
{
	// ensure exits are sorted
	sort_exits();
	
	return 0;
}

/* save the stupid thing */
void
Room::save (File::Writer& writer)
{
	writer.attr("id", id);

	if (!name.empty())
		writer.attr("name", name.get_name());

	if (!desc.empty())
		writer.attr("desc", desc);
	
	Entity::save(writer);

	if (flags.outdoors)
		writer.attr("outdoors", "yes");
	if (flags.safe)
		writer.attr("safe", "yes");
	if (flags.noweather)
		writer.attr("noweather", "yes");

	if (coins)
		writer.attr("coins", coins);

	for (EList<RoomExit>::const_iterator i = exits.begin(); i != exits.end(); ++i) {
		writer.begin("exit");
		(*i)->save(writer);
		writer.end();
	}

	for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i) {
		writer.begin("object");
		(*i)->save (writer);
		writer.end();
	}

	for (EList<Character>::const_iterator i = chars.begin(); i != chars.end(); ++i) {
		if (NPC(*i)) {
			writer.begin("npc");
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

RoomExit *
Room::find_exit (const char *e_name, uint c, uint *matches)
{
	assert (e_name);
	assert (c != 0);

	if (matches)
		*matches = 0;

	for (EList<RoomExit>::const_iterator i = exits.begin(); i != exits.end(); ++i) {
		if ((*i)->name_match (e_name)) {
			if (matches)
				++ *matches;
			if ((-- c) == 0)
				return (*i);
		}
	}
	return NULL;
}

RoomExit *
Room::get_exit_by_dir (ExitDir dir)
{
	for (EList<RoomExit>::const_iterator i = exits.begin(); i != exits.end(); ++i)
		if ((*i)->get_dir () == dir)
			return (*i);
	return NULL;
}

RoomExit *
Room::new_exit (void)
{
	RoomExit *exit = new RoomExit ();
	if (exit == NULL)
		return NULL;
	exit->parent_room = this;
	exit->activate();
	exits.add(exit);
	return exit;
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
Room::heartbeat (void)
{
	// call update hook
	Hooks::room_heartbeat(this);
}

void
Room::activate (void)
{
	Entity::activate ();

	for (EList<RoomExit>::const_iterator i = exits.begin(); i != exits.end(); ++i)
		(*i)->activate();
	for (EList<Character>::const_iterator i = chars.begin(); i != chars.end(); ++i)
		(*i)->activate();
	for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i)
		(*i)->activate();
}

void
Room::deactivate (void)
{
	for (EList<RoomExit>::const_iterator i = exits.begin(); i != exits.end(); ++i)
		(*i)->deactivate();
	for (EList<Character>::const_iterator i = chars.begin(); i != chars.end(); ++i)
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
Room::get_owner (void) const
{
	return zone;
}

void
Room::owner_release (Entity* child)
{
	// Character?
	Character* ch = CHARACTER(child);
	if (ch != NULL) {
		chars.remove(ch);
		return;
	}

	// Object?
	Object* obj = OBJECT(child);
	if (obj != NULL) {
		objects.remove(obj);
		return;
	}

	// Exit?
	RoomExit* exit = ROOMEXIT(child);
	if (exit != NULL) {
		exits.remove(exit);
		return;
	}

	// something we don't support
	assert(false);
}

/* print out Room */
void
Room::show (const StreamControl& stream, Character* viewer)
{
	// if there's a hook for this, don't do our version
	if (Hooks::show_room(this, viewer))
		return;

	// basic info
	stream << "[ " << StreamName(*this, NONE, true) << " ]\n";
	stream << CDESC "  " << StreamParse(get_desc(), "room", this, "actor", viewer) << CNORMAL;


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

	// exit list
	if (!exits.empty()) {
		// setup
		int displayed = 0;
		RoomExit* last = NULL;
		
		// iterator
		for (EList<RoomExit>::const_iterator i = exits.begin(); i != exits.end(); ++i) {
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

	// displaying characters and objects
	int displayed = 0;
	Entity* last = NULL;

	// show players and NPCs
	if (!chars.empty()) {
		// iterator
		for (EList<Character>::const_iterator i = chars.begin(); i != chars.end(); ++i) {
			// not ourselves
			if ((Character*)(*i) != viewer) {
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

/* print all exits */
void
Room::show_exits (const StreamControl& stream)
{
	for (EList<RoomExit>::const_iterator i = exits.begin(); i != exits.end(); ++i)
		stream << StreamName(*(*i)) << " <" << (*i)->get_target() << ">\n";
}

/* broadcast a message to the Room */
void
Room::put (const char *msg, size_t len, GCType::vector<Character*>* ignore_list)
{
	assert (msg != NULL);

	// iterator
	for (EList<Character>::iterator i = chars.begin(); i != chars.end(); ++i) {
		// skip ignored characters
		if (ignore_list != NULL) {
			if (std::find(ignore_list->begin(), ignore_list->end(), (*i)) != ignore_list->end())
				continue;
		}
		// output
		(*i)->stream_put(msg, len);
	}
}

/* find a Character by name */
Character *
Room::find_character (const char *cname, uint c, uint *matches)
{
	assert (cname != NULL);
	assert (c != 0);
	
	return CHARACTER(chars.match (cname, c, matches));
}

/* find an object by name */
Object *
Room::find_object (const char *oname, uint c, uint *matches)
{
	assert (oname != NULL);
	assert (c != 0);

	return OBJECT(objects.match (oname, c, matches));
}

void
Room::add_character (Character* character)
{
	assert(character != NULL);

	character->set_owner(this);
	chars.add (character);

	// initialize NPC AI for new NPCs
	if (NPC(character)) {
		if (((Npc*)character)->get_ai())
			((Npc*)character)->get_ai()->do_load(character);
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
Room::count_players (void) const
{
	unsigned long count = 0;
	for (EList<Character>::const_iterator i = chars.begin(); i != chars.end(); ++i)
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
	EList<Entity> children(objects.size() + chars.size() + exits.size());
	size_t index = 0;

	// propogate to objects
	for (EList<Object>::const_iterator i = objects.begin(); i != objects.end(); ++i)
		children[index++] = *i;

	// propogate to characters
	for (EList<Character>::const_iterator i = chars.begin(); i != chars.end(); ++i)
		children[index++] = *i;

	// propogate to exits
	for (EList<RoomExit>::const_iterator i = exits.begin(); i != exits.end(); ++i)
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
Room::sort_exits (void)
{
	std::sort(exits.begin(), exits.end(), DerefSort<RoomExit>());
}
