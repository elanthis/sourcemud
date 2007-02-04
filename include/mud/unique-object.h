/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef MUD_UNIQUE_OBJECT_H
#define MUD_UNIQUE_OBJECT_H

#include "mud/object.h"

// Object control
class
UniqueObject : public Object
{
	public:
	UniqueObject ();

	virtual String factory_type () const { return S("uobject"); }

	// name info
	bool set_name (String);
	virtual EntityName get_name () const { return name; }

	// description
	virtual String get_desc () const { return desc; }
	void set_desc (String s_desc) { desc = s_desc; }

	// save/load
	virtual int load_node (File::Reader& reader, File::Node& node);
	virtual void save_data (File::Writer& writer);

	// weight
	uint get_real_weight () const { return weight; }
	void set_weight (uint s_weight) { weight = s_weight; }

	// cost
	uint get_cost () const { return cost; }
	void set_cost (uint s_cost) { cost = s_cost; }

	// equip slot
	EquipSlot get_equip () const { return equip; }
	void set_equip (EquipSlot s_equip) { equip = s_equip; }

	// check flags
	bool get_flag (ObjectFlag flag) const { return flags & flag; }
	void set_flag (ObjectFlag flag, bool b) { flags.set(flag, b); }
	bool is_hidden () const;
	bool is_touchable () const;
	bool is_gettable () const;
	bool is_dropable () const;
	bool is_trashable () const;
	bool is_rotting () const;

	// containers
	bool has_location (ObjectLocation type) const { return locations & type; }

	// data
	private:
	EntityName name;
	String desc;
	uint weight;
	uint cost;
	EquipSlot equip;
	StringList keywords;
	TagList tags;

	// flags
	BitSet<ObjectFlag> flags;

	// locations
	BitSet<ObjectLocation> locations;

	protected:
	virtual ~UniqueObject ();
};

#endif