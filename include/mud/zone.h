/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_ZONE_H
#define SOURCEMUD_MUD_ZONE_H

#include "mud/entity.h"
#include <vector>
#include "mud/server.h"
#include "common/imanager.h"

// announce flags
enum AnnounceFlags {
	ANFL_NONE = (0),
	ANFL_OUTDOORS = (1 << 0),
	ANFL_INDOORS = (1 << 1),
};

class SZoneManager;
class Zone;

class Spawn 
{
	protected:
	TagID tag;
	StringList blueprints;
	StringList rooms;
	uint min;
	uint delay;
	uint dcount;

	public:
	Spawn () : tag(), blueprints(), rooms(), min(1), delay(0), dcount(0) {}

	bool check (const class Zone* zone) const;
	void spawn (class Zone* zone) const;
	bool heartbeat ();

	int load (File::Reader& reader);
	void save (File::Writer& writer) const;
};

class Zone
{
	public:
	Zone ();

	// zone ID
	String get_id () const { return id; }
	void set_id (String new_id) { id = new_id; }

	// name information
	String get_name () const { return name; }
	void set_name (String s_name) { name = s_name; }

	// find rooms
	class Room* get_room (String name) const;
	class Room* get_room_at (size_t index) const;
	size_t get_room_count () const;

	// manage rooms
	void add_room (class Room*);

	// events
	void broadcast_event (const Event& event);

	// load/save
	int load (String path);
	void save ();

	// announce to all rooms
	void announce (String text, AnnounceFlags type = ANFL_NONE) const;

	// update zone
	void heartbeat ();

	// (de)activate children rooms
	void activate ();
	void deactivate ();

	// delete the zone
	void destroy ();

	protected:
	String id;
	String name;

	typedef std::vector<class Room*> RoomList;
	RoomList rooms;

	typedef std::vector<Spawn> SpawnList;
	SpawnList spawns;

	friend class SZoneManager;
};

class SZoneManager : public IManager
{
	public:
	virtual int initialize ();
	virtual void shutdown ();
	virtual void save ();

	// load the world
	int load_world ();

	// lookup entries
	Zone* get_zone (String);
	Zone* get_zone_at (size_t index);
	class Room* get_room (String);

	// send an announcement to all the rooms in all the zones
	void announce (String, AnnounceFlags = ANFL_NONE);

	// add a new zone
	void add_zone (Zone*);

	// show all rooms
	void list_rooms (const class StreamControl& stream);

	private:
	typedef std::vector<Zone*> ZoneList;
	ZoneList zones;

	friend void Zone::destroy ();
};
extern SZoneManager ZoneManager;

#define ZONE(ent) E_CAST(ent,Zone)

#endif
