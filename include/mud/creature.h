/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_CREATURE_H
#define AWEMUD_MUD_CREATURE_H

#include "mud/entity.h"
#include "common/types.h"
#include "common/string.h"
#include "mud/body.h"
#include "common/streams.h"
#include "common/bitset.h"

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
class CreatureStatID {
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
	
	CreatureStatID (int s_value) : value((type_t)s_value) {}
	CreatureStatID () : value(NONE) {}

	String get_name() const { return names[value]; }
	String get_short_name() const { return short_names[value]; }

	type_t get_value () const { return value; }

	static CreatureStatID lookup (String name);

	bool operator == (CreatureStatID dir) const { return dir.value == value; }
	bool operator != (CreatureStatID dir) const { return dir.value != value; }
	operator bool () const { return value != NONE; }

	private:
	type_t value;

	static String names[];
	static String short_names[];
};

// character stat array
class CreatureStatArray {
	public:
	inline const uint16 operator[] (CreatureStatID stat) const { return stat ? stats[stat.get_value()] : 0; }
	inline uint16& operator[] (CreatureStatID stat) { return stats[stat ? stat.get_value() : 0]; }

	private:
	uint16 stats[CreatureStatID::COUNT];
};

// Creature Position object
class CreaturePosition {
	public:
	typedef enum {
		STAND = 0,
		SIT,
		LAY,
		KNEEL,
		COUNT
	} type_t;
	
	inline CreaturePosition (int s_value) : value((type_t)s_value) {}
	inline CreaturePosition () : value(STAND) {}

	inline String get_name() const { return names[value]; }
	inline String get_verb() const { return verbs[value]; }
	inline String get_sverb() const { return sverbs[value]; }
	inline String get_verbing() const { return verbings[value]; }

	inline type_t get_value () const { return value; }

	static CreaturePosition lookup (String name);

	inline bool operator == (CreaturePosition dir) const { return dir.value == value; }
	inline bool operator != (CreaturePosition dir) const { return dir.value != value; }

	private:
	type_t value;

	static String names[];
	static String verbs[];
	static String sverbs[]; // sits vs. sit, etc.
	static String verbings[]; // sitting vs. sit, etc.
};

// Creature control
class
Creature : public Entity, public IStreamSink
{
	public:
	Creature (const Scriptix::TypeInfo* type);

	// save/load
	virtual int load_node (File::Reader& reader, File::Node& node);
	virtual int load_finish ();
	virtual void save_data (File::Writer& writer);
	virtual void save_hook (class ScriptRestrictedWriter* writer);

	// streaming in Scriptix
	IStreamSink* get_stream () { return this; }

	// positon
	CreaturePosition get_pos () const { return position; }
	CreaturePosition set_pos (CreaturePosition p) { return position = p; }

	// health
	inline int get_hp () const { return health.cur; }
	inline int set_hp (int new_hp) { return health.cur = new_hp; } // NOTE: avoid use of, only necessary in rare cases
	inline int get_max_hp () const { return health.max; }
	inline int set_max_hp (int new_mhp) { return health.max = new_mhp; } // NOTE: avoid use of

	// check data
	inline bool is_dead () const { return dead; }

	// gender
	virtual GenderType get_gender () const = 0;

	// stats
	virtual int get_base_stat (CreatureStatID stat) const = 0;
	inline int get_effective_stat (CreatureStatID stat) const { assert (stat); return effective_stats[stat.get_value()]; }
	inline void set_effective_stat (CreatureStatID stat, int val) { assert (stat); effective_stats[stat.get_value()] = val; }
	int get_stat_modifier (CreatureStatID stat) const;

	// combat
	virtual uint get_combat_dodge () const = 0; // dodge skill
	virtual uint get_combat_attack () const = 0; // attack accuracy
	virtual uint get_combat_damage () const = 0; // damage factor

	// events
	virtual void handle_event (const Event& event);
	virtual void broadcast_event (const Event& event);

	// equipment
	int hold (class Object*);
	int wear (class Object*);
	int equip (class Object*);

	bool is_held (class Object*) const;
	bool is_worn (class Object*) const;
	bool is_equipped (class Object*) const;

	void drop_held (class Room*);
	void drop_all (class Room*);

	class Object* get_held_at (uint index) const;
	class Object* get_worn_at (uint index) const;
	class Object* get_equip_at (uint index) const;

	class Object* get_held_by_loc (uint loc) const;
	class Object* get_worn_by_loc (uint loc) const;
	class Object* get_equip_by_loc (uint loc) const;

	class Object* find_held (String name, uint count = 1, uint* matches = NULL) const;
	class Object* find_worn (String name, uint count = 1, uint* matches = NULL) const;
	class Object* find_equip (String name, uint count = 1, uint* matches = NULL) const;

	void release_object (class Object*); // *ONLY* for use by Object::release() !!!!

	// hands
	int free_hands () const;
	void swap_hands ();

	// currency
	inline uint get_coins () const { return coins; }
	inline uint set_coins (uint amount) { return coins = amount; }
	uint give_coins (uint amount);
	uint take_coins (uint amount);

	// health
	void heal (uint amount);
	bool damage (uint amount, Creature *attacker); // returns true if damage killed
	virtual void kill (Creature *killer) = 0;

	// Creature abilities
	inline bool can_move () const { return !is_dead(); }
	inline bool can_see () const { return true; }
	inline bool can_talk () const { return true; }
	inline bool can_act () const { return !is_dead(); }

	// affects
	int add_affect (class CreatureAffectGroup* affect);

	// actions
	void add_action (IAction* action);
	IAction* get_action () const;
	void cancel_action ();

	// round time
	uint get_round_time () const;

	// action checks w/ error messages
	bool check_alive (); // must be alive
	bool check_move (); // can move
	bool check_rt (); // roundtime has expired
	bool check_see (); // can see stuff

	// input/output
	virtual void stream_put (const char*, size_t len) {};
	virtual void process_command (String);

	// command processing utility funcs
	class Object* cl_find_object (String name, int type, bool silent = false);
	class Object* cl_find_object (String name, class Object* container, class ObjectLocation container, bool silent = false);

	class Creature* cl_find_creature (String name, bool silent = false);
	class Portal* cl_find_portal (String name, bool silent = false);
	/* cl_find_any looks for a creature, then an object, then an portal.
	 * Object searching is the same as using cl_find_object w/ GOC_ANY. */
	class Entity* cl_find_any (String name, bool silent = false);

	// heartbeat
	virtual void heartbeat ();

	// must (de)activate equipment
	virtual void activate ();
	virtual void deactivate ();

	// owner - see entity.h
	virtual void set_owner (Entity* s_owner);
	virtual void owner_release (Entity* child);
	virtual class Entity* get_owner () const;
	inline class Room* get_room () const { return location; }

	// enter a room
	bool enter (class Room*, class Portal *in_portal);

	// recalculate stuff
	virtual void recalc_stats ();
	virtual void recalc_health ();
	virtual void recalc ();

	// parsing
	virtual int macro_property (const class StreamControl& stream, String method, const MacroList& argv) const;

	// output description of character or equipment lsit
	void display_equip (const class StreamControl& stream) const;

	// output a list of affects
	void display_affects (const class StreamControl& stream) const;

	// == ACTIONS ==
	void do_emote (String text);
	void do_social (const class Social* social, Entity* target, String adverb);
	void do_say (String text);
	void do_sing (String text);

	void do_look ();
	void do_look (Creature *who);
	void do_look (class Object *what, class ObjectLocation container);
	void do_look (class Portal *what);

	void do_move (int dir);

	void do_position (CreaturePosition);

	void do_get (class Object*, class Object*, class ObjectLocation container);
	void do_put (class Object*, class Object*, class ObjectLocation container);
	void do_give_coins (class Creature* target, uint amount);
	void do_drop (class Object*);

	void do_wear (class Object*);
	void do_remove (class Object*);

	void do_read (class Object*);
	void do_kick(class Object*);
	void do_eat (class Object*);
	void do_drink (class Object*);
	void do_raise (class Object*);
	void do_touch (class Object*);

	void do_open (class Portal*);
	void do_close (class Portal*);
	void do_unlock (class Portal*);
	void do_lock (class Portal*);
	void do_kick (class Portal*);

	void do_attack (class Creature*);
	void do_kill (class Creature*);

	void do_go (class Portal*);

	// == DATA ITEMS ==
	protected:
	typedef GCType::vector<IAction*> ActionList;
	typedef GCType::vector<CreatureAffectGroup*> AffectStatusList;

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

	protected:
	E_TYPE(Creature)
};

// stream out character descriptions
class StreamCreatureDesc {
	public:
	inline StreamCreatureDesc (Creature* s_ch) : ch(s_ch) {}

	inline friend const StreamControl&
	operator << (const StreamControl& stream, const StreamCreatureDesc& desc)
	{
		desc.ch->display_desc(stream);
		return stream;
	}

	private:
	Creature* ch;
};


String get_stat_level (uint);

#define CHARACTER(ent) E_CAST(ent,Creature)

#endif
