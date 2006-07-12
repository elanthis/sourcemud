/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef PLAYER_H
#define PLAYER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <vector>
#include <algorithm>

#include "common/string.h"
#include "mud/char.h"
#include "mud/color.h"
#include "mud/network.h"
#include "mud/pdesc.h"
#include "mud/gametime.h"
#include "common/gcbase.h"
#include "mud/skill.h"
#include "common/gcvector.h"
#include "common/gcmap.h"
#include "common/imanager.h"
#include "mud/bindings.h"

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif // HAVE_LIBZ

// player name length requirements
#define PLAYER_NAME_MIN_LEN 3
#define PLAYER_NAME_MAX_LEN 15

enum exp_spec {
	EXP_GENERAL = 0,
	EXP_WARRIOR,
	EXP_ROGUE,
	EXP_CASTER,
	NUM_EXPS
};

class Player : public Character
{
	public:
	// create
	Player (class Account* s_account, StringArg s_id);

	// player's unique ID (this is identical to their name)
	inline String get_id () const { return name.get_text(); }

	// name information (special for players)
	inline virtual EntityName get_name () const { return name; }

	// description information
	virtual inline String get_desc () const { return String(); }
	virtual inline void set_desc (StringArg s_desc) {}

	// gender
	virtual inline GenderType get_gender () const { return pdesc.gender; }
	inline void set_gender (GenderType s_gender) { pdesc.gender = s_gender; }

	// alignment
	virtual inline CharAlign get_alignment () const { return alignment; }
	inline void set_alignment (CharAlign s_align) { alignment = s_align; }
	inline CharAlign adjust_alignment (int mod) { set_alignment(CharAlign(get_alignment() + mod)); return get_alignment(); }

	// save and load
	virtual void save (File::Writer& writer);
	virtual void save_hook (class ScriptRestrictedWriter* writer);
	void save ();

	int load_node (File::Reader& reader, File::Node& node);

	// output color
	inline String ncolor () const { return S(CPLAYER); }

	// account
	inline class Account* get_account () const { return account; }

	// connected?
	//   currently in use by someone
	inline bool is_connected () const { return conn != NULL; }

	// birthday/age
	uint get_age () const;
	inline GameTime get_birthday () const { return birthday; }
	inline void set_birthday (const GameTime& gt) { birthday = gt; }

	// misc
	void kill (Character* killer);
	void heartbeat ();

	// manage account active counts
	virtual void activate ();
	virtual void deactivate ();

	// race
	inline class Race* get_race () const { return race; }
	inline class Race* set_race (class Race* race_in) { return race = race_in; }

	// height in centimeters
	inline uint8 get_height () const { return pdesc.height; }
	inline void set_height (uint8 height) { pdesc.height = height; }

	// character traits
	CharacterTraitValue get_trait(CharacterTraitID) const;
	void set_trait (CharacterTraitID, CharacterTraitValue);

	// combat
	virtual uint get_combat_dodge () const;
	virtual uint get_combat_attack () const;
	virtual uint get_combat_damage () const;

	// class/exp
	inline uint get_exp (uint type) const { if (type < NUM_EXPS) return exp[type]; else return 0; }
	void grant_exp (uint type, uint amount);

	// stats
	virtual inline int get_base_stat (CharStatID stat) const { return base_stats[stat.get_value()]; }
	inline void set_base_stat (CharStatID stat, int value) { base_stats[stat.get_value()] = value; }

	// recalcuate everything
	virtual void recalc_stats ();
	virtual void recalc ();

	// display info
	void display_inventory ();
	virtual void display_desc (const class StreamControl& stream) const;
	void display_skills ();

	// I/O
	virtual void stream_put (const char* data, size_t len = 0);
	void show_prompt ();
	void process_command (String cmd);
	void connect (class TelnetHandler* telnet);
	void disconnect ();
	inline class TelnetHandler* get_telnet() const { return conn; }
	void toggle_echo (bool value);
	void set_indent (uint level);
	uint get_width ();
	void clear_scr ();

	// parsing
	virtual int parse_property (const class StreamControl& stream, StringArg method, const ParseArgs& argv) const;

	// handling "player states"
	int start (); // start the session
	void quit (); // save and exit

	// player-only actions
	void do_tell (Player* who, StringArg what);
	void do_reply (StringArg what);

	protected:
	typedef GCType::map<CharacterTraitID, CharacterTraitValue> TraitMap;

	EntityName name;
	class TelnetHandler* conn;
	Account* account;
	String last_command;
	String last_tell;
	struct PDesc {
		GenderType gender;
		uint8 height; // centimeters;
		TraitMap traits;
	} pdesc;
	CharStatArray base_stats;
	class Race *race;
	CharAlign alignment;
	uint exp[NUM_EXPS];
	GameTime birthday;
	SkillSet skills;
	struct NetworkInfo {
		uint last_rt; // last reported round-time
		uint last_max_rt; // last reported max round-time
		int last_hp; // last reported hp 
		int last_max_hp; // last reported hp 
		uint timeout_ticks; // remaining ticks until timeout
	} ninfo;

#ifdef HAVE_LIBZ
	// compression
	bool begin_mccp ();
	void end_mccp ();
#endif // HAVE_LIBZ

	E_SUBTYPE(Player,Character);

	friend class SPlayerManager; // obvious

	protected:
	~Player ();
};

// manage all players
class SPlayerManager : public IManager
{
	public:
	// list of *connected* players - do *not* use GC
	typedef std::list<Player*> PlayerList;

	// initialize the player manager
	virtual int initialize ();

	// shutdown the player manager
	virtual void shutdown ();

	// true if 'name' is a valid player name
	bool valid_name (String name);

	// return the path a player's file is at
	String path (StringArg name);

	// return the logged-in player with the given name
	Player* get (StringArg name);

	// load a player - from disk
	Player* load (class Account* account, StringArg name);

	// DESTROY a player permanently (with backup)
	int destroy (StringArg name);

	// does a valid player of this name exist?
	bool exists (String name);

	// count of connected players
	size_t count ();

	// list all connected players
	void list (const StreamControl& stream);

	// save all players
	void save ();

	// EEEEW - return list of all players - yuck
	inline const PlayerList& get_player_list () { return player_list; }

	private:
	PlayerList player_list;

	// yuck - let Player class manage their own membership
	friend class Player;
};
extern SPlayerManager PlayerManager;

#define PLAYER(ent) E_CAST((ent),Player)

extern String get_stat_level (uint value);
extern String get_stat_color (uint value);

#endif
