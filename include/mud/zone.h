/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_ZONE_H
#define SOURCEMUD_MUD_ZONE_H

#include "common/imanager.h"
#include "mud/entity.h"
#include "mud/server.h"

// announce flags
enum AnnounceFlags {
	ANFL_NONE = (0),
	ANFL_OUTDOORS = (1 << 0),
	ANFL_INDOORS = (1 << 1),
};

class _MZone;
class Zone;

class Spawn
{
protected:
	TagID tag;
	std::vector<std::string> blueprints;
	std::vector<std::string> rooms;
	uint min;
	uint delay;
	uint dcount;

public:
	Spawn() : tag(), blueprints(), rooms(), min(1), delay(0), dcount(0) {}

	bool check(const class Zone* zone) const;
	void spawn(class Zone* zone) const;
	bool heartbeat();

	int load(File::Reader& reader);
	void save(File::Writer& writer) const;
};

class Zone
{
public:
	Zone();

	// zone ID
	std::string getId() const { return id; }
	void setId(const std::string& new_id) { id = new_id; }

	// name information
	std::string getName() const { return name; }
	void setName(const std::string& s_name) { name = s_name; }

	// find rooms
	class Room* getRoom(const std::string& name) const;
	class Room* getRoomAt(size_t index) const;
	size_t getRoomCount() const;

	// manage rooms
	void addRoom(class Room*);

	// events
	void broadcastEvent(const Event& event);

	// load/save
	int load(const std::string& path);
	void save();

	// announce to all rooms
	void announce(const std::string& text, AnnounceFlags type = ANFL_NONE) const;

	// update zone
	void heartbeat();

	// (de)activate children rooms
	void activate();
	void deactivate();

	// delete the zone
	void destroy();

protected:
	std::string id;
	std::string name;

	typedef std::vector<class Room*> RoomList;
	RoomList rooms;

	typedef std::vector<Spawn> SpawnList;
	SpawnList spawns;

	friend class _MZone;
};

class _MZone : public IManager
{
public:
	virtual int initialize();
	virtual void shutdown();
	virtual void save();

	// load the world
	int loadWorld();

	// lookup entries
	Zone* getZone(const std::string&);
	Zone* getZoneAt(size_t index);
	class Room* getRoom(const std::string&);

	// send an announcement to all the rooms in all the zones
	void announce(const std::string&, AnnounceFlags = ANFL_NONE);

	// add a new zone
	void addZone(Zone*);

	// show all rooms
	void listRooms(const class StreamControl& stream);

private:
	typedef std::vector<Zone*> ZoneList;
	ZoneList zones;

	friend void Zone::destroy();
};
extern _MZone MZone;

#define ZONE(ent) E_CAST(ent,Zone)

#endif
