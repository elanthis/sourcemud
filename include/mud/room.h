/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_ROOM_H
#define AWEMUD_MUD_ROOM_H

#include "mud/elist.h"
#include "common/string.h"
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
	EList<Portal> portals;

	Room ();

	// name information
	inline virtual EntityName get_name () const { return name; }
	inline void set_name (String s_name) { name.set_name(s_name); }

	// description information
	inline virtual String get_desc () const { return desc; }
	inline virtual void set_desc (String s_desc) { desc = s_desc; }

	// outdoors
	inline bool is_outdoors () const { return flags.outdoors; }
	inline void set_outdoors (bool v) { flags.outdoors = v; }

	// safe (no combat/attacks)
	inline bool is_safe () const { return flags.safe; }
	inline void set_safe (bool v) { flags.safe = v; }

	// no weather notices in room descriptions (but does get global weather changes)
	inline bool is_noweather () const { return flags.noweather; }
	inline void set_noweather (bool v) { flags.noweather = v; }

	// portals
	class Portal* get_portal_at (uint);
	class Portal* get_portal_by_dir (PortalDir);
	class Portal* find_portal (String, uint c = 1, uint *matches = NULL);
	class Portal* new_portal (); //  will pick a unique ID, return portal
	void sort_portals (); // re-sort portals; FIXME: this is ugly to have to do manually

	// identifier
	inline String get_id () const { return id; }
	inline void set_id (String new_id) { id = new_id; }

	// colour type
	inline virtual String ncolor () const { return S(CTITLE); }

	// io
	virtual void save (File::Writer& writer);
	virtual void save_hook (class ScriptRestrictedWriter* writer);
	virtual int load_node(File::Reader& reader, File::Node& node);
	virtual int load_finish ();

	// streaming
	IStreamSink* get_stream ();

	// heartbeat
	void heartbeat ();

	// (de)activate children
	virtual void activate ();
	virtual void deactivate ();

	// display
	void show (const class StreamControl& stream, class Creature* viewer);
	void show_portals (const class StreamControl& stream);

	// output
	void put (String text, size_t len, GCType::vector<class Creature*>* ignore = NULL);

	// get entities
	class Creature* find_creature (String name, uint c = 1, uint *matches = NULL);
	class Object* find_object (String name, uint c = 1, uint *matches = NULL);

	// count players in room
	unsigned long count_players () const;

	// add entities
	void add_object (class Object* object);
	void add_creature (class Creature* creature);

	// coins on the floor
	inline uint get_coins () const { return coins; }
	uint take_coins (uint amount);
	uint give_coins (uint amount);

	// handle events - propogate
	virtual int handle_event (const Event& event);

	// owner management - see entity.h
	virtual void set_owner (Entity* owner);
	virtual void owner_release (Entity* child);
	virtual class Entity* get_owner () const;
	inline class Zone* get_zone () const { return zone; }

	protected:
	String id;
	EntityName name;
	String desc;
	class Zone* zone;
	uint coins;
	struct RoomFlags {
		char outdoors:1, safe:1, noweather:1;
	} flags;

	protected:
	~Room ();

	E_TYPE(Room)
	
	friend class SZoneManager;
};

#define ROOM(ent) E_CAST(ent,Room)

#endif
