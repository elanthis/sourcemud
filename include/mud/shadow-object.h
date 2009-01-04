/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef MUD_SHADOW_OBJECT_H
#define MUD_SHADOW_OBJECT_H

#include "mud/object.h"

// Object blueprint
class
			ObjectBP
{
public:
	ObjectBP();

	// blueprint id
	inline std::string get_id() const { return id; }

	// name
	virtual EntityName get_name() const;
	bool set_name(const std::string& s_name);

	const std::vector<std::string>& get_keywords() const { return keywords; }

	// description
	const std::string& get_desc() const { return desc; }
	void set_desc(const std::string& s_desc) { desc = s_desc; }

	// weight
	uint get_weight() const { return weight; }
	void set_weight(uint s_weight) { weight = s_weight; }

	// cost
	uint get_cost() const { return cost; }
	void set_cost(uint s_cost) { cost = s_cost; }

	// container
	bool has_location(ObjectLocation type) const { return locations & type; }

	// equip location
	EquipSlot get_equip() const { return equip; }
	void set_equip(EquipSlot s_equip) { equip = s_equip; }

	// flags
	bool get_flag(ObjectFlag flag) const { return flags & flag; }
	void set_flag(ObjectFlag flag, bool b) { flags.set(flag, b); }

	// tags
	bool has_tag(TagID tag) const;
	int add_tag(TagID tag);
	int remove_tag(TagID tag);
	inline const TagList& get_tags() const { return tags; }

	// load
	int load(File::Reader& reader);
	void save(File::Writer& writer);

private:
	std::string id;
	EntityName name;
	std::string desc;
	uint weight;
	uint cost;
	EquipSlot equip;
	std::vector<std::string> keywords;
	TagList tags;

	// flags
	BitSet<ObjectFlag> flags;

	// locations
	BitSet<ObjectLocation> locations;
};

// Object control
class
			ShadowObject : public Object
{
public:
	ShadowObject();
	ShadowObject(ObjectBP* s_blueprint);

	virtual const char* factory_type() const { return "sobject"; }

	// name info
	bool set_name(const std::string&);
	virtual EntityName get_name() const;
	virtual bool name_match(const std::string& name) const;

	// description
	virtual std::string get_desc() const;

	// save/load
	virtual int load_node(File::Reader& reader, File::Node& node);
	virtual int load_finish();
	virtual void save_data(File::Writer& writer);

	// weight
	uint get_real_weight() const;

	// object properties
	uint get_cost() const;
	EquipSlot get_equip() const;

	// check flags
	bool get_flag(ObjectFlag flag) const { return blueprint->get_flag(flag); }
	bool is_hidden() const;
	bool is_touchable() const;
	bool is_gettable() const;
	bool is_dropable() const;
	bool is_trashable() const;
	bool is_rotting() const;

	// return ture if we derive from the named blueprint
	bool is_blueprint(const std::string& blueprint) const;

	// blueprint information
	virtual ObjectBP* get_blueprint() const { return blueprint; }
	void set_blueprint(ObjectBP* blueprint);
	static Object* load_blueprint(const std::string& name);

	// containers
	bool has_location(ObjectLocation type) const { return blueprint->has_location(type); }

	// data
private:
	EntityName name;
	ObjectBP* blueprint;

protected:
	virtual ~ShadowObject();
};

class _MObjectBP : public IManager
{
	typedef std::map<std::string, ObjectBP*> BlueprintMap;

public:
	int initialize();

	void shutdown();

	ObjectBP* lookup(const std::string& id);

private:
	BlueprintMap blueprints;
};

extern _MObjectBP MObjectBP;

#define OBJECT(ent) E_CAST(ent,Object)

#endif
