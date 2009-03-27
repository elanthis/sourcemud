/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_CREATURE_H
#define SOURCEMUD_MUD_CREATURE_H

#include "common/types.h"
#include "common/streams.h"
#include "mud/body.h"
#include "mud/entity.h"

class IAction;

// for get_comm_obj
#define GOC_HELD (1<<0)
#define GOC_WORN (1<<1)
#define GOC_EQUIP (GOC_HELD|GOC_WORN)
#define GOC_FLOOR (1<<2)
#define GOC_SUB (1<<3)
#define GOC_ROOM (GOC_FLOOR|GOC_SUB)
#define GOC_ANY (GOC_ROOM|GOC_EQUIP)

// Creature statistic ID
class CreatureStatID
{
public:
	typedef enum {
		NONE = -1,
		STRENGTH = 0,
		AGILITY,
		FORTITUDE,
		INTELLECT,
		SPIRIT,
		WILLPOWER,
		COUNT,
	} type_t;

	CreatureStatID(int s_value) : value((type_t)s_value) {}
	CreatureStatID() : value(NONE) {}

	std::string getName() const { return names[value]; }
	std::string getShortName() const { return short_names[value]; }

	type_t getValue() const { return value; }

	static CreatureStatID lookup(const std::string& name);

	bool operator == (CreatureStatID dir) const { return dir.value == value; }
	bool operator != (CreatureStatID dir) const { return dir.value != value; }
	operator bool () const { return value != NONE; }

private:
	type_t value;

	static std::string names[];
	static std::string short_names[];
};

// character stat array
class CreatureStatArray
{
public:
	inline const uint16 operator[](CreatureStatID stat) const { return stat ? stats[stat.getValue()] : 0; }
	inline uint16& operator[](CreatureStatID stat) { return stats[stat ? stat.getValue() : 0]; }

private:
	uint16 stats[CreatureStatID::COUNT];
};

// Creature Position object
class CreaturePosition
{
public:
	typedef enum {
		STAND = 0,
		SIT,
		LAY,
		KNEEL,
		COUNT
	} type_t;

	inline CreaturePosition(int s_value) : value((type_t)s_value) {}
	inline CreaturePosition() : value(STAND) {}

	inline std::string getName() const { return names[value]; }
	inline std::string getPassiveVerb() const { return passive_verbs[value]; }
	inline std::string getActiveVerb() const { return active_verbs[value]; }
	inline std::string getState() const { return states[value]; }

	inline type_t getValue() const { return value; }

	static CreaturePosition lookup(const std::string& name);

	inline bool operator == (CreaturePosition dir) const { return dir.value == value; }
	inline bool operator != (CreaturePosition dir) const { return dir.value != value; }

private:
	type_t value;

	static std::string names[];
	static std::string passive_verbs[];
	static std::string active_verbs[];
	static std::string states[];
};

// Creature control
class Creature : public Entity, public IStreamSink
{
public:
	Creature();

	// save/load
	virtual int loadNode(File::Reader& reader, File::Node& node);
	virtual int loadFinish();
	virtual void saveData(File::Writer& writer);
	virtual void saveHook(File::Writer& writer);

	// streaming
	IStreamSink* getStream() { return this; }

	// positon
	CreaturePosition getPosition() const { return position; }
	CreaturePosition setPosition(CreaturePosition p) { return position = p; }

	// health
	inline int getHP() const { return health.cur; }
	inline int setHP(int new_hp) { return health.cur = new_hp; }  // NOTE: avoid use of, only necessary in rare cases
	inline int getMaxHP() const { return health.max; }
	inline int setMaxHP(int new_mhp) { return health.max = new_mhp; }  // NOTE: avoid use of

	// check data
	inline bool isDead() const { return dead; }

	// gender
	virtual GenderType getGender() const = 0;

	// stats
	virtual int getBaseStat(CreatureStatID stat) const = 0;
	inline int getEffectiveStat(CreatureStatID stat) const { assert(stat); return effective_stats[stat.getValue()]; }
	inline void setEffectiveStat(CreatureStatID stat, int val) { assert(stat); effective_stats[stat.getValue()] = val; }
	int getStatModifier(CreatureStatID stat) const;

	// combat
	virtual uint getCombatDodge() const = 0;  // dodge skill
	virtual uint getCombatAttack() const = 0;  // attack accuracy
	virtual uint getCombatDamage() const = 0;  // damage factor

	// events
	virtual void handleEvent(const Event& event);
	virtual void broadcastEvent(const Event& event);

	// equipment
	int hold(class Object*);
	int wear(class Object*);
	int equip(class Object*);

	bool isHeld(class Object*) const;
	bool isWorn(class Object*) const;
	bool isEquipped(class Object*) const;

	void dropHeld(class Room*);
	void dropAll(class Room*);

	class Object* getHeldAt(uint index) const;
	class Object* getWornAt(uint index) const;
	class Object* getEquipAt(uint index) const;

	class Object* getHeldByLoc(uint loc) const;
	class Object* getWornByLoc(uint loc) const;
	class Object* getEquipByLoc(uint loc) const;

	class Object* findHeld(const std::string& name, uint count = 1, uint* matches = NULL) const;
	class Object* findWorn(const std::string& name, uint count = 1, uint* matches = NULL) const;
	class Object* findEquip(const std::string& name, uint count = 1, uint* matches = NULL) const;

	void releaseObject(class Object*);  // *ONLY* for use by Object::release() !!!!

	// hands
	int freeHands() const;
	void swapHands();

	// currency
	inline uint getCoins() const { return coins; }
	inline uint setCoins(uint amount) { return coins = amount; }
	uint giveCoins(uint amount);
	uint takeCoins(uint amount);

	// health
	void heal(uint amount);
	bool damage(uint amount, Creature *attacker);  // returns true if damage killed
	virtual void kill(Creature *killer) = 0;

	// Creature abilities
	inline bool canMove() const { return !isDead(); }
	inline bool canSee() const { return true; }
	inline bool canTalk() const { return true; }
	inline bool canAct() const { return !isDead(); }

	// affects
	int addAffect(class CreatureAffectGroup* affect);

	// actions
	void addAction(IAction* action);
	IAction* getAction() const;
	void cancelAction();

	// round time
	uint getRoundTime() const;

	// action checks w/ error messages
	bool checkAlive();  // must be alive
	bool checkMove();  // can move
	bool checkRound();  // roundtime has expired
	bool checkSee();  // can see stuff

	// input/output
	virtual void streamPut(const char*, size_t len) {};
	virtual void processCommand(const std::string&);

	// command processing utility funcs
	class Object* clFindObject(const std::string& name, int type, bool silent = false);
	class Object* clFindObject(const std::string& name, class Object* container, class ObjectLocation loc, bool silent = false);

	class Creature* clFindCreature(const std::string& name, bool silent = false);
	class Portal* clFindPortal(const std::string& name, bool silent = false);
	/* clFindAny looks for a creature, then an object, then an portal.
	 * Object searching is the same as using clFindObject w/ GOC_ANY. */
	class Entity* clFindAny(const std::string& name, bool silent = false);

	// heartbeat
	virtual void heartbeat();

	// must (de)activate equipment
	virtual void activate();
	virtual void deactivate();

	// owner - see entity.h
	virtual void setOwner(Entity* owner);
	virtual void ownerRelease(Entity* child);
	virtual class Entity* getOwner() const;
	inline class Room* getRoom() const { return location; }

	// enter a room
	bool enter(class Room*, class Portal *in_portal);

	// recalculate stuff
	virtual void recalcStats();
	virtual void recalcHealth();
	virtual void recalc();

	// parsing
	virtual int macroProperty(const class StreamControl& stream, const std::string& method, const MacroList& argv) const;

	// output description of character or equipment lsit
	void displayEquip(const class StreamControl& stream) const;

	// output a list of affects
	void displayAffects(const class StreamControl& stream) const;

	// == ACTIONS ==
	void doEmote(const std::string& text);
	void doSay(const std::string& text);
	void doSing(const std::string& text);

	void doLook();
	void doLook(Creature *who);
	void doLook(class Object *what, class ObjectLocation container);
	void doLook(class Portal *what);

	void doMove(int dir);

	void doPosition(CreaturePosition);

	void doGet(class Object*, class Object*, class ObjectLocation container);
	void doPut(class Object*, class Object*, class ObjectLocation container);
	void doGiveCoins(class Creature* target, uint amount);
	void doDrop(class Object*);

	void doWear(class Object*);
	void doRemove(class Object*);

	void doRead(class Object*);
	void doKick(class Object*);
	void doEat(class Object*);
	void doDrink(class Object*);
	void doRaise(class Object*);
	void doTouch(class Object*);

	void doOpen(class Portal*);
	void doClose(class Portal*);
	void doUnlock(class Portal*);
	void doLock(class Portal*);
	void doKick(class Portal*);

	void doAttack(class Creature*);
	void doKill(class Creature*);

	void doGo(class Portal*);

	// == DATA ITEMS ==
protected:
	typedef std::vector<IAction*> ActionList;
	typedef std::vector<CreatureAffectGroup*> AffectStatusList;

	struct {
		class Object* left_held;
		class Object* right_held;
		class Object* body_worn;
		class Object* back_worn;
		class Object* waist_worn;
	} equipment;
	struct {
		int16 cur;
		uint16 max;
	} health; // hit points
	bool dead;
	CreaturePosition position;
	CreatureStatArray effective_stats;
	uint round_time; // round time
	uint coins;
	class Room* location;
	ActionList actions;
	AffectStatusList affects;
};

// stream out character descriptions
class StreamCreatureDesc
{
public:
	inline StreamCreatureDesc(Creature* s_ch) : ch(s_ch) {}

	inline friend const StreamControl&
	operator << (const StreamControl& stream, const StreamCreatureDesc& desc) {
		desc.ch->displayDesc(stream);
		return stream;
	}

private:
	Creature* ch;
};

std::string getStatLevel(uint);

#define CHARACTER(ent) E_CAST(ent,Creature)

#endif
