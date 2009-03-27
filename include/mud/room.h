/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_ROOM_H
#define SOURCEMUD_MUD_ROOM_H

#include "mud/elist.h"
#include "mud/portal.h"

class Object;
class Portal;
class Creature;

// Room, the general type
class Room : public Entity
{
public:
	// nasty public stuff
	EList<Object> objects;
	EList<Creature> creatures;
	std::map<PortalDir, Portal*> portals;

	Room();

	virtual const char* factoryType() const { return "room"; }

	// name information
	inline virtual EntityName getName() const { return name; }
	inline void setName(const std::string& s_name) { name.setFull(s_name); }

	// description information
	inline virtual std::string getDesc() const { return desc; }
	inline virtual void setDesc(const std::string& s_desc) { desc = s_desc; }

	// outdoors
	inline bool isOutdoors() const { return flags.outdoors; }
	inline void setOutdoors(bool v) { flags.outdoors = v; }

	// safe (no combat/attacks)
	inline bool isSafe() const { return flags.safe; }
	inline void setSafe(bool v) { flags.safe = v; }

	// no weather notices in room descriptions (but does get global weather changes)
	inline bool isNoweather() const { return flags.noweather; }
	inline void setNoweather(bool v) { flags.noweather = v; }

	// portals
	class Portal* getPortalAt(uint);
	class Portal* getPortalByDir(PortalDir);
	class Portal* findPortal(const std::string&, uint c = 1, uint *matches = NULL);
	class Portal* newPortal(PortalDir dir);

	// this is called when a portal needs to attach itself to the room
	// as a target
	bool registerPortal(Portal* portal);
	void unregisterPortal(Portal* portal);

	// identifier
	inline std::string getId() const { return id; }
	inline void setId(const std::string& new_id) { id = new_id; }

	// colour type
	virtual const char* ncolor() const { return CTITLE; }

	// io
	virtual void saveData(File::Writer& writer);
	virtual void saveHook(File::Writer& writer);
	virtual int loadNode(File::Reader& reader, File::Node& node);
	virtual int loadFinish();

	// streaming
	IStreamSink* getStream();

	// heartbeat
	void heartbeat();

	// (de)activate children
	virtual void activate();
	virtual void deactivate();

	// display
	void show(const class StreamControl& stream, class Creature* viewer);
	void showPortals(const class StreamControl& stream);

	// output
	void put(const std::string& text, size_t len, std::vector<class Creature*>* ignore = NULL);

	// get entities
	class Creature* findCreature(const std::string& name, uint c = 1, uint *matches = NULL);
	class Object* findObject(const std::string& name, uint c = 1, uint *matches = NULL);

	// count players in room
	unsigned long countPlayers() const;

	// add entities
	void addObject(class Object* object);
	void addCreature(class Creature* creature);

	// coins on the floor
	inline uint getCoins() const { return coins; }
	uint takeCoins(uint amount);
	uint giveCoins(uint amount);

	// events
	virtual void handleEvent(const Event& event);
	virtual void broadcastEvent(const Event& event);

	// owner management - see entity.h
	virtual void setOwner(Entity* owner);
	virtual void ownerRelease(Entity* child);
	virtual class Entity* getOwner() const;

	void setZone(Zone* s_zone) { zone = s_zone; }
	inline class Zone* getZone() const { return zone; }

protected:
	std::string id;
	EntityName name;
	std::string desc;
	class Zone* zone;
	uint coins;
	struct RoomFlags {
char outdoors:
1, safe:
1, noweather:
		1;
	} flags;

protected:
	~Room();

	friend class _MZone;
};

#define ROOM(ent) E_CAST(ent,Room)

#endif
