/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef OBJECT_H
#define OBJECT_H

#include "mud/entity.h"
#include "common/string.h"
#include "common/error.h"
#include "mud/body.h"
#include "mud/elist.h"
#include "mud/container.h"
#include "mud/server.h"
#include "common/imanager.h"
#include "common/gcmap.h"
#include "scriptix/native.h"
#include "scriptix/function.h"
#include "common/bitset.h"

// Object flags
enum {
	OBJ_FLAG_TRASH = 1,
	OBJ_FLAG_HIDDEN,
	OBJ_FLAG_ROT,
	OBJ_FLAG_TOUCH,
	OBJ_FLAG_GET,
	OBJ_FLAG_DROP,

	OBJ_FLAG_MAX
};

// Object blueprint "set" flags
enum {
	OBJBL_SET_NAME = 1,
	OBJBL_SET_DESC,
	OBJBL_SET_WEIGHT,
	OBJBL_SET_COST,
	OBJBL_SET_EQUIP,

	OBJBL_SET_MAX
};

// WEIGHT:
//  cost is in hundredths of 1 unit of current (ex: US dollar)
//  so a cost of 2100 would possibly equal 21 US$.  In most
//  fantasy settings, gold pieces or silver pieces are the standard
//  unit of currency, so 2100 might equal 21 gold or 21 silver.

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

// Object blueprint
class
ObjectBlueprint : public Scriptix::Native
{
	public:
	typedef GCType::set<ContainerType> ContainerList;

	ObjectBlueprint ();

	// blueprint id
	inline String get_id () const { return id; }

	// name
	virtual EntityName get_name () const;
	bool set_name (String s_name);
	void reset_name ();

	const StringList& get_keywords () const { return keywords; }

	// description
	const String& get_desc () const { return desc; }
	void set_desc (String s_desc) { desc = s_desc; value_set &= OBJBL_SET_DESC; }
	void reset_desc ();

	// weight
	uint get_weight () const { return weight; }
	void set_weight (uint s_weight) { weight = s_weight; value_set &= OBJBL_SET_WEIGHT; }
	void reset_weight ();

	// cost
	uint get_cost () const { return cost; }
	void set_cost (uint s_cost) { cost = s_cost; value_set &= OBJBL_SET_COST; }
	void reset_cost ();

	// equip location
	EquipLocation get_equip () const { return equip; }
	void set_equip (EquipLocation s_equip) { equip = s_equip; value_set &= OBJBL_SET_EQUIP; }
	void reset_equip ();

	// flags
	bool get_flag (bit_t flag) const { return flags & flag; }
	void set_flag (bit_t flag, bool b) { flags.set(flag, b); flags_set &= flag; }
	void reset_flag (bit_t flag);

	// update inherited data
	void refresh ();

	// containers
	const ContainerList& get_containers () const { return containers; }
	bool set_container_exist (ContainerType type, bool);
	bool has_container (ContainerType type) const;

	// load
	int load (File::Reader& reader);
	void save (File::Writer& writer);

	// parent blueprint
	virtual ObjectBlueprint* get_parent () const { return parent; }

	private:
	String id;
	ObjectBlueprint* parent;
	
	ContainerList containers;

	EntityName name;
	String desc;
	uint weight;
	uint cost;
	EquipLocation equip;
	StringList keywords;

	// flags
	BitSet<OBJ_FLAG_MAX-1> flags;

	// mark whether some item is "set" or not
	BitSet<OBJ_FLAG_MAX-1> flags_set;
	BitSet<OBJBL_SET_MAX-1> value_set;

	void set_parent (ObjectBlueprint* blueprint);

	virtual Scriptix::Value get_undefined_property (Scriptix::Atom id) const;
};

// Object control
class
Object : public Entity
{
	public:
	Object ();
	Object (ObjectBlueprint* s_blueprint);

	// name info
	bool set_name (String);
	virtual EntityName get_name () const;
	virtual bool name_match (String name) const;

	// description
	virtual String get_desc () const;

	// save/load
	virtual int load_node (File::Reader& reader, File::Node& node);
	virtual int load_finish ();
	virtual void save (File::Writer& writer);
	virtual void save_hook (class ScriptRestrictedWriter* writer);

	// weight
	inline uint get_weight () const { return calc_weight + get_real_weight(); }
	uint get_real_weight () const;

	// owner tracking - see entity.h
	virtual inline Entity* get_owner () const { return owner; }
	virtual void set_owner (Entity* s_owner);
	virtual void owner_release (Entity* child);

	// returns the room the object is in (tracing through parents) or the
	// character holding the object (again, tracing through parenst)
	class Creature* get_holder () const;
	class Room* get_room () const;

	// name color
	virtual String ncolor () const { return S(CITEM); }

	// for parsing, pull a property based on a char*
	virtual int parse_property (const class StreamControl& stream, String method, const ParseList& argv) const;

	// object properties
	uint get_cost () const;
	EquipLocation get_equip () const;

	// check flags
	bool is_hidden () const;
	bool is_touchable () const;
	bool is_gettable () const;
	bool is_dropable () const;
	bool is_trashable () const;
	bool is_rotting () const;

	// return ture if we derive from the named blueprint
	bool is_blueprint (String blueprint) const;

	// (de)activate children
	virtual void activate ();
	virtual void deactivate ();

	// heartbeat
	void heartbeat ();

	// blueprint information
	virtual ObjectBlueprint* get_blueprint () const { return blueprint; }
	void set_blueprint (ObjectBlueprint* blueprint);
	static Object* load_blueprint (String name);

	// containers
	bool has_container (ContainerType type) const;
	bool add_object (Object *sub, ContainerType type);
	void remove_object (Object *sub, ContainerType type);
	Object *find_object (String name, uint index, ContainerType type, uint *matches = NULL) const;
	void show_contents (class Player *player, ContainerType type) const;

	// data
	private:
	EntityName name;
	Entity *owner;
	ObjectBlueprint* blueprint;
	ContainerType location;
	uint calc_weight; // calculated weight of children objects
	uint trash_timer; // ticks until trashed

	EList<Object> children; // child objects

	// weight tracking
	void recalc_weight ();

	protected:
	virtual ~Object ();

	virtual Scriptix::Value get_undefined_property (Scriptix::Atom id) const;

	E_TYPE(Object)
};

class SObjectBlueprintManager : public IManager
{
	typedef GCType::map<String,ObjectBlueprint*> BlueprintMap;

	public:
	int initialize ();

	void shutdown ();

	ObjectBlueprint* lookup (String id);

	private:
	BlueprintMap blueprints;
};

extern SObjectBlueprintManager ObjectBlueprintManager;

#define OBJECT(ent) E_CAST(ent,Object)

#endif
