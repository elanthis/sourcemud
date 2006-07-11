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
#include "mud/exit.h"

class Object;
class RoomExit;
class Character;

// Room, the general type
class Room : public Entity
{
	public:
	// nasty public stuff
	EList<Object> objects;
	EList<Character> chars;
	EList<RoomExit> exits;

	Room (void);

	// name information
	inline virtual EntityName get_name (void) const { return name; }
	inline void set_name (StringArg s_name) { name.set_name(s_name); }

	// description information
	inline virtual String get_desc (void) const { return desc; }
	inline virtual void set_desc (StringArg s_desc) { desc = s_desc; }

	// outdoors
	inline bool is_outdoors (void) const { return flags.outdoors; }
	inline void set_outdoors (bool v) { flags.outdoors = v; }

	// safe (no combat/attacks)
	inline bool is_safe (void) const { return flags.safe; }
	inline void set_safe (bool v) { flags.safe = v; }

	// no weather notices in room descriptions (but does get global weather changes)
	inline bool is_noweather (void) const { return flags.noweather; }
	inline void set_noweather (bool v) { flags.noweather = v; }

	// exits
	class RoomExit* get_exit_at (uint);
	class RoomExit* get_exit_by_dir (ExitDir);
	class RoomExit* find_exit (StringArg, uint c = 1, uint *matches = NULL);
	class RoomExit* new_exit (void); //  will pick a unique ID, return exit
	void sort_exits (void); // re-sort exits; FIXME: this is ugly to have to do manually

	// identifier
	inline StringArg get_id (void) const { return id; }
	inline void set_id (StringArg new_id) { id = new_id; }

	// colour type
	inline virtual const char* ncolor (void) const { return CTITLE; }

	// io
	virtual void save (File::Writer& writer);
	virtual void save_hook (class ScriptRestrictedWriter* writer);
	virtual int load_node(File::Reader& reader, File::Node& node);
	virtual int load_finish (void);

	// heartbeat
	void heartbeat (void);

	// (de)activate children
	virtual void activate (void);
	virtual void deactivate (void);

	// display
	void show (const class StreamControl& stream, class Character* viewer);
	void show_exits (const class StreamControl& stream);

	// output
	void put (StringArg text, size_t len, GCType::vector<class Character*>* ignore = NULL);

	// get entities
	class Character* find_character (StringArg name, uint c = 1, uint *matches = NULL);
	class Object* find_object (StringArg name, uint c = 1, uint *matches = NULL);

	// count players in room
	unsigned long count_players (void) const;

	// add entities
	void add_object (class Object* object);
	void add_character (class Character* character);

	// coins on the floor
	inline uint get_coins (void) const { return coins; }
	uint take_coins (uint amount);
	uint give_coins (uint amount);

	// handle events - propogate
	virtual int handle_event (const Event& event);

	// owner management - see entity.h
	virtual void set_owner (Entity* owner);
	virtual void owner_release (Entity* child);
	virtual class Entity* get_owner (void) const;
	inline class Zone* get_zone (void) const { return zone; }

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
	~Room (void);

	E_TYPE(Room)
	
	friend class SZoneManager;
};

#define ROOM(ent) E_CAST(ent,Room)

#endif
