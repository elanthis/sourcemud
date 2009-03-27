/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef OBJECT_H
#define OBJECT_H

#include "common/error.h"
#include "common/imanager.h"
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

	ObjectFlag(bits b) : v(b) {}
	explicit ObjectFlag(int i) : v((bits)(i >= 1 && i < MAX ? i : 1)) {}
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

	ObjectLocation(bits b) : v(b) {}
	explicit ObjectLocation(int i) : v((bits)(i >= 1 && i < MAX ? i : 1)) {}
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

// Object blueprint
class
ObjectBP
{
public:
	ObjectBP();

	// blueprint id
	inline std::string getId() const { return id; }
	inline bool isAnonymous() const { return id.empty(); }

	// name
	virtual EntityName getName() const;
	bool setName(const std::string& s_name);

	const std::vector<std::string>& getKeywords() const { return keywords; }

	// description
	const std::string& getDesc() const { return desc; }
	void setDesc(const std::string& s_desc) { desc = s_desc; }

	// weight
	uint getWeight() const { return weight; }
	void setWeight(uint s_weight) { weight = s_weight; }

	// cost
	uint getCost() const { return cost; }
	void setCost(uint s_cost) { cost = s_cost; }

	// container
	bool hasLocation(ObjectLocation type) const { return locations.test(type); }

	// equip location
	EquipSlot getEquip() const { return equip; }
	void setEquip(EquipSlot s_equip) { equip = s_equip; }

	// flags
	bool getFlag(ObjectFlag flag) const { return flags.test(flag); }
	void setFlag(ObjectFlag flag, bool b) { flags.set(flag, b); }

	// tags
	bool hasTag(TagID tag) const;
	int addTag(TagID tag);
	int removeTag(TagID tag);
	inline const TagList& getTags() const { return tags; }

	// load
	int load(File::Reader& reader);
	void save(File::Writer& writer);

private:
	std::string id;
	std::string desc;
	EntityName name;
	uint weight;
	uint cost;
	EquipSlot equip;
	std::vector<std::string> keywords;
	TagList tags;

	// flags
	std::bitset<ObjectFlag::MAX> flags;

	// locations
	std::bitset<ObjectLocation::MAX> locations;
};

// Object control
class
Object : public Entity
{
public:
	Object();
	Object(ObjectBP* s_blueprint);

	// factory name
	virtual const char* factoryType() const { return "object"; }

	// return ture if we derive from the named blueprint
	bool isBlueprint(const std::string& blueprint) const;

	// blueprint information
	virtual ObjectBP* getBlueprint() const { return blueprint; }
	void setBlueprint(ObjectBP* blueprint);
	static Object* loadBlueprint(const std::string& name);

	// name info
	bool setName(const std::string&);
	EntityName getName() const;
	bool nameMatch(const std::string& name) const;

	// description
	std::string getDesc() const;

	// save/load
	virtual int loadNode(File::Reader& reader, File::Node& node);
	virtual int loadFinish();
	virtual void saveData(File::Writer& writer);
	virtual void saveHook(File::Writer& writer);

	// weight
	inline uint getWeight() const { return calc_weight + getRealWeight(); }
	uint getRealWeight() const;

	// owner tracking - see entity.h
	virtual inline Entity* getOwner() const { return owner; }
	virtual void setOwner(Entity* s_owner);
	virtual void ownerRelease(Entity* child);

	// events
	virtual void handleEvent(const Event& event);
	virtual void broadcastEvent(const Event& event);

	// returns the room the object is in (tracing through parents) or the
	// character holding the object (again, tracing through parenst)
	class Creature* getHolder() const;
	class Room* getRoom() const;

	// name color
	virtual const char* ncolor() const { return CITEM; }

	// for parsing, pull a property based on a char*
	virtual int macroProperty(const class StreamControl& stream, const std::string& method, const MacroList& argv) const;

	// object properties
	uint getCost() const;
	EquipSlot getEquip() const;

	// check flags
	bool getFlag(ObjectFlag flag) const { return blueprint->getFlag(flag); }
	bool isHidden() const { return getFlag(ObjectFlag::HIDDEN); }
	bool isTouchable() const { return getFlag(ObjectFlag::TOUCH); }
	bool isGettable() const { return getFlag(ObjectFlag::GET); }
	bool isDropable() const { return getFlag(ObjectFlag::DROP); }
	bool isTrashable() const { return getFlag(ObjectFlag::TRASH); }
	bool isRotting() const { return getFlag(ObjectFlag::ROT); }

	// (de)activate children
	virtual void activate();
	virtual void deactivate();

	// heartbeat
	void heartbeat();

	// containers
	bool hasLocation(ObjectLocation type) const { return blueprint->hasLocation(type); }
	bool addObject(Object *sub, ObjectLocation type);
	void removeObject(Object *sub, ObjectLocation type);
	Object *findObject(const std::string& name, uint index, ObjectLocation type, uint *matches = NULL) const;
	void showContents(class Player *player, ObjectLocation type) const;

	// data
private:
	Entity *owner;
	EntityName name;
	ObjectBP* blueprint;
	ObjectLocation in_container; // the type of container this object is in
	uint calc_weight; // calculated weight of children objects
	uint trash_timer; // ticks until trashed

	EList<Object> children; // child objects

	// weight tracking
	void recalcWeight();

protected:
	virtual ~Object();
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
