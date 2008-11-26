/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include <time.h>

#include "mud/room.h"
#include "mud/zone.h"
#include "mud/settings.h"
#include "common/rand.h"
#include "mud/npc.h"
#include "common/streams.h"
#include "mud/clock.h"
#include "mud/gametime.h"
#include "mud/weather.h"
#include "mud/object.h"
#include "mud/hooks.h"
#include "common/manifest.h"
#include "mud/shadow-object.h"

SZoneManager ZoneManager;

bool
Spawn::check (const Zone* zone) const
{
	if (!tag.valid())
		return false;
	size_t count = EntityManager.tag_count(tag);
	return count < min;
}

bool
Spawn::heartbeat()
{
	// ready for update?
	if (++dcount >= delay) {
		dcount = 0;
		return true;
	}

	return false;
}

void
Spawn::spawn (Zone* zone) const
{
	// FIXME: print warnings for the returns below;
	//        builders should know about this stuff...

	// any blueprints to spawn?
	if (blueprints.empty())
		return;
	// any rooms to spawn in?
	if (rooms.empty())
		return;

	// select random room
	String roomname = rooms[get_random(rooms.size())];

	// find room
	Room* room = zone->get_room(roomname);
	if (room == NULL)
		return;

	// select random blueprint id
	String tempname = blueprints[get_random(blueprints.size())];

	// try to spawn as NPC
	Npc* npc = Npc::load_blueprint(tempname);
	if (npc != NULL) {
		// make sure NPC has the tag
		npc->add_tag(tag);

		// zone lock the NPC
		npc->set_zone_locked(true);

		// add to room
		npc->enter(room, NULL);
	} else {
		// try to spawn as object
		Object* object = ShadowObject::load_blueprint(tempname);
		if (object != NULL) {
			// make sure object has the tag
			object->add_tag(tag);

			// add to room
			room->add_object(object);
		}
	}
}

int
Spawn::load (File::Reader& reader)
{
	min = 1;
	tag = TagID();
	blueprints.resize(0);
	rooms.resize(0);
	delay = 1;
	dcount = 0;

	FO_READ_BEGIN
		FO_ATTR("spawn", "count")
			min = node.get_int();
		FO_ATTR("spawn", "tag")
			tag = TagID::create(node.get_string());
		FO_ATTR("spawn", "delay")
			delay = node.get_int();
		FO_ATTR("spawn", "blueprint")
			blueprints.push_back(node.get_string());
		FO_ATTR("spawn", "room")
			rooms.push_back(node.get_string());
	FO_READ_ERROR
		return -1;
	FO_READ_END

	return 0;
}


void
Spawn::save (File::Writer& writer) const
{
	writer.begin(S("zone"), S("spawn"));

	writer.attr(S("spawn"), S("tag"), TagID::nameof(tag));
	writer.attr(S("spawn"), S("count"), min);
	writer.attr(S("spawn"), S("delay"), delay);
	for (StringList::const_iterator i = blueprints.begin(); i != blueprints.end(); ++i) {
		writer.attr(S("spawn"), S("blueprint"), *i);
	}
	for (StringList::const_iterator i = rooms.begin(); i != rooms.end(); ++i) {
		writer.attr(S("spawn"), S("room"), *i);
	}

	writer.end();
}

Zone::Zone()
{}

Room*
Zone::get_room (String id) const
{
	for (RoomList::const_iterator i = rooms.begin(); i != rooms.end(); ++i)
		if (str_eq ((*i)->get_id(), id))
			return (*i);

	return NULL;
}

Room*
Zone::get_room_at (size_t index) const
{
	for (RoomList::const_iterator i = rooms.begin(); i != rooms.end(); ++i)
		if (index-- == 0)
			return (*i);

	return NULL;
}

size_t
Zone::get_room_count() const
{
	return rooms.size();
}

int
Zone::load (String path)
{
	File::Reader reader;
	if (reader.open(path))
		return -1;

	FO_READ_BEGIN
		FO_ATTR("zone", "name")
			set_name(node.get_string());
		FO_ATTR("zone", "id")
			id = node.get_string();
		FO_ENTITY("zone", "child")
			if (ROOM(entity) == NULL) throw File::Error(S("Zone child is not a Room"));
			add_room(ROOM(entity));
		FO_OBJECT("zone", "spawn")
			Spawn spawn;
			if (!spawn.load (reader))
				spawns.push_back(spawn);
			else
				throw File::Error(S("Failed to load room"));
	FO_READ_ERROR
		return -1;
	FO_READ_END

	return 0;
}

void
Zone::save()
{
	String path = SettingsManager.get_zone_path() + "/" + get_id() + ".zone";

	/* backup zone file */
	if (SettingsManager.get_backup_zones()) {
		char time_buffer[15];
		time_t base_t;
		time (&base_t);
		strftime (time_buffer, sizeof (time_buffer), "%Y%m%d%H%M%S", localtime (&base_t));
		String backup = path + S(".") + String(time_buffer) + S("~");
		if (rename (path, backup)) /* move file */
			Log::Error << "Backup of zone '" << get_id() << "' to " << backup << " failed: " << strerror(errno);
	}

	File::Writer writer;
	if (writer.open(path)) {
		Log::Error << "Failed to open " << path << " for writing";
		return;
	}

	// header
	writer.comment(S("Zone: ") + get_id());

	// basics
	writer.bl();
	writer.comment (S("--- BASICS ---"));
	writer.attr(S("zone"), S("id"), id);
	writer.attr(S("zone"), S("name"), name);

	// spawns
	writer.bl();
	writer.comment (S("--- SPAWNS ---"));
	for (SpawnList::const_iterator i = spawns.begin(); i != spawns.end(); ++i)
		i->save(writer);

	// rooms
	writer.bl();
	writer.comment(S("--- ROOMS ---"));
	for (RoomList::iterator i = rooms.begin(); i != rooms.end(); ++i)
		(*i)->save(writer, S("zone"), S("child"));

	writer.bl();
	writer.comment (S(" --- EOF ---"));
}

void
Zone::add_room (Room *room)
{
	assert (room != NULL);

	room->set_zone(this);
	rooms.push_back(room);
}

void
Zone::heartbeat()
{
	// spawn systems
	for (SpawnList::iterator i = spawns.begin(); i != spawns.end(); ++i) {
		if (i->heartbeat())
			if (i->check(this))
				i->spawn(this);
	}
}

void
Zone::activate()
{
	for (RoomList::iterator i = rooms.begin(); i != rooms.end(); ++i)
		(*i)->activate();
}

void
Zone::deactivate()
{
	for (RoomList::iterator i = rooms.begin(); i != rooms.end(); ++i)
		(*i)->deactivate();
}

void
Zone::destroy()
{
	SZoneManager::ZoneList::iterator i = find(ZoneManager.zones.begin(), ZoneManager.zones.end(), this);
	if (i != ZoneManager.zones.end())
		ZoneManager.zones.erase(i);

	rooms.clear();
	spawns.clear();

	deactivate();
}

void
Zone::broadcast_event (const Event& event)
{
	for (RoomList::iterator i = rooms.begin(); i != rooms.end(); ++i)
		EventManager.resend(event, *i);
}

int
SZoneManager::initialize()
{
	// modules we need
	if (require(UniqueIDManager) != 0)
		return 1;
	if (require(EntityManager) != 0)
		return 1;
	if (require(NpcBPManager) != 0)
		return 1;
	if (require(ObjectBPManager) != 0)
		return 1;
	if (require(TimeManager) != 0)
		return 1;
	if (require(WeatherManager) != 0)
		return 1;
	return 0;
}

// load the world
int
SZoneManager::load_world()
{
	// read zones dir
	ManifestFile man(SettingsManager.get_zone_path(), S(".zone"));
	StringList files = man.get_files();;
	for (StringList::iterator i = files.begin(); i != files.end(); ++i) {
		Zone* zone = new Zone();
		if (zone->load (*i))
			return -1;
		zones.push_back(zone); // don't call add_zone(), we don't want to activate it yet
	}

	// now activate all zones
	for (ZoneList::iterator i = zones.begin(); i != zones.end(); ++i)
		(*i)->activate();

	return 0;
}

// close down zone manager
void
SZoneManager::shutdown()
{
	// deactive all zones, which deactives all entities.
	// we don't delete them until a collection is run, in order to
	// protect against any portals/rooms/whatever that might link
	// back to the zone
	for (ZoneList::iterator i = zones.begin(), e = zones.end(); i != e; ++i)
		(*i)->deactivate();

	// collect entities
	EntityManager.collect();

	// delete zones
	for (ZoneList::iterator i = zones.begin(), e = zones.end(); i != e; ++i)
		delete *i;
	zones.clear();
}

// save zones
void
SZoneManager::save()
{
	for (ZoneList::iterator i = zones.begin(); i != zones.end(); ++i)
		(*i)->save();
}

/* find a Zone */
Zone*
SZoneManager::get_zone (String id)
{
	assert (id);

	for (ZoneList::iterator i = zones.begin(); i != zones.end(); ++i)
		if (str_eq ((*i)->get_id(), id))
			return (*i);

	return NULL;
}

/* get a Zone  by index */
Zone*
SZoneManager::get_zone_at (size_t index)
{
	if (index >= zones.size())
		return NULL;

	return zones[index];
}

/* find a Room */
Room *
SZoneManager::get_room (String id)
{
	if (id.empty())
		return NULL;

	Room *room;
	for (ZoneList::iterator i = zones.begin(); i != zones.end(); ++i) {
		room = (*i)->get_room (id);
		if (room != NULL)
			return room;
	}

	return NULL;
}

void
Zone::announce (String str, AnnounceFlags flags) const
{
	for (RoomList::const_iterator i = rooms.begin(); i != rooms.end(); ++i) {
		if (!flags ||
			(flags & ANFL_OUTDOORS && (*i)->is_outdoors()) ||
			(flags & ANFL_INDOORS && !(*i)->is_outdoors())
		)
			**i << str << "\n";
	}
}

/* announce to all rooms in a Room */
void
SZoneManager::announce (String str, AnnounceFlags flags)
{
	for (ZoneList::iterator i = zones.begin(); i != zones.end(); ++i)
		(*i)->announce (str, flags);
}

void
SZoneManager::add_zone (Zone *zone)
{
	assert (zone != NULL);

	// make sure we don't already have the zone
	for (ZoneList::iterator i = zones.begin(); i != zones.end(); ++i)
		if (*i == zone)
			return;

	// push the zone
	zones.push_back(zone);

	// activate it
}

void
SZoneManager::list_rooms (const StreamControl& stream)
{
	for (ZoneList::iterator i = zones.begin(); i != zones.end(); ++i) {
		stream << " " << (*i)->get_name() << " <" << (*i)->get_id() << ">\n";
		for (Zone::RoomList::iterator ii = (*i)->rooms.begin(); ii != (*i)->rooms.end(); ++ii)
			stream << "   " << StreamName(*ii) << " <" << (*ii)->get_id() << ">\n";
	}
}
