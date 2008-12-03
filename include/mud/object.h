/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef OBJECT_H
#define OBJECT_H

#include "common.h"
#include "common/error.h"
#include "common/imanager.h"
#include "common/bitset.h"
#include "mud/entity.h"
#include "mud/body.h"
#include "mud/elist.h"
#include "mud/server.h"

// Object flags
struct ObjectFlag {
	enum bits {
		TRASH = 1,
		HIDDEN,
		ROT,
		TOUCH,
		GET,
		DROP,

		MAX
	};

	ObjectFlag (bits b) : v(b) {}
	explicit ObjectFlag (int i) : v((bits)(i>=1&&i<MAX?i:1)) {}
	operator bits() const { return v; }

	bits v;
};

// Object container
struct ObjectLocation {
	enum bits {
		NONE = 1,
		IN,
		ON,

		MAX
	};

	ObjectLocation (bits b) : v(b) {}
	explicit ObjectLocation (int i) : v((bits)(i>=1&&i<MAX?i:1)) {}
	ObjectLocation() : v(NONE) {}
	operator bits() const { return v; }

	std::string name() const { return names[v-1]; }
	static std::string names[MAX];

	bits v;
};

// COST:
//  cost is in hundredths of 1 unit of current (ex: US dollar)
//  so a cost of 2100 would possibly equal 21 US$.  In most
//  fantasy settings, gold pieces or silver pieces are the standard
//  unit of currency, so 2100 might equal 21 gold or 21 silver.

// WEIGHT:
//  weight is measured in decagrams.  There are 100 decagrams to a
//  kilogram.  For the imperialists out there (such as myself),
//  there are roughly 4.453 kilograms to each pound.  Most fantasy
//  games might want to convert all weights in stones; there are
//  14 pounds to a stone, or 16.350 kilograms to a stone.  To
//  convert weight to pounds, divide by 445.3.  To convert weight
//  to stones, divide weigth by 6234.2.

// TRASH:
//  the trash timer should be configurable...

#define OBJECT_TRASH_TICKS ROUNDS_TO_TICKS(60 * 20)
  // trash objects after 20 minutes
#define OBJECT_ROT_TICKS ROUNDS_TO_TICKS(60 * 1)
  // rotting objects wither away after 1 minute

// Object control
class
Object : public Entity
{
	public:
	Object();

	// name info
	virtual bool set_name (const std::string&) = 0;
	virtual EntityName get_name() const = 0;

	// description
	virtual std::string get_desc() const = 0;

	// save/load
	virtual int load_node (File::Reader& reader, File::Node& node);
	virtual int load_finish();
	virtual void save_data (File::Writer& writer);
	virtual void save_hook (File::Writer& writer);

	// weight
	inline uint get_weight() const { return calc_weight + get_real_weight(); }
	virtual uint get_real_weight() const = 0;

	// owner tracking - see entity.h
	virtual inline Entity* get_owner() const { return owner; }
	virtual void set_owner (Entity* s_owner);
	virtual void owner_release (Entity* child);

	// events
	virtual void handle_event (const Event& event);
	virtual void broadcast_event (const Event& event);

	// returns the room the object is in (tracing through parents) or the
	// character holding the object (again, tracing through parenst)
	class Creature* get_holder() const;
	class Room* get_room() const;

	// name color
	virtual const char* ncolor() const { return CITEM; }

	// for parsing, pull a property based on a char*
	virtual int macro_property (const class StreamControl& stream, const std::string& method, const MacroList& argv) const;

	// object properties
	virtual uint get_cost() const = 0;
	virtual EquipSlot get_equip() const = 0;

	// check flags
	virtual bool get_flag (ObjectFlag flag) const = 0;
	bool is_hidden() const { return get_flag(ObjectFlag::HIDDEN); }
	bool is_touchable() const { return get_flag(ObjectFlag::TOUCH); }
	bool is_gettable() const { return get_flag(ObjectFlag::GET); }
	bool is_dropable() const { return get_flag(ObjectFlag::DROP); }
	bool is_trashable() const { return get_flag(ObjectFlag::TRASH); }
	bool is_rotting() const { return get_flag(ObjectFlag::ROT); }

	// (de)activate children
	virtual void activate();
	virtual void deactivate();

	// heartbeat
	void heartbeat();

	// containers
	virtual bool has_location (ObjectLocation type) const = 0;
	bool add_object (Object *sub, ObjectLocation type);
	void remove_object (Object *sub, ObjectLocation type);
	Object *find_object (const std::string& name, uint index, ObjectLocation type, uint *matches = NULL) const;
	void show_contents (class Player *player, ObjectLocation type) const;

	// data
	private:
	Entity *owner;
	ObjectLocation container;
	uint calc_weight; // calculated weight of children objects
	uint trash_timer; // ticks until trashed

	EList<Object> children; // child objects

	// weight tracking
	void recalc_weight();

	protected:
	virtual ~Object();

	E_TYPE(Object)
};

#define OBJECT(ent) E_CAST(ent,Object)

#endif
