/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "common/imanager.h"
#include "common/time.h"
#include "mud/creature.h"
#include "mud/color.h"
#include "mud/form.h"
#include "mud/gametime.h"
#include "mud/skill.h"
#include "mud/pconn.h"
#include "net/socket.h"

// player name length requirements
#define PLAYER_NAME_MIN_LEN 3
#define PLAYER_NAME_MAX_LEN 15

class Player : public Creature
{
public:
	// create and initialize
	Player(std::tr1::shared_ptr<class Account> s_account, const std::string& s_id);

	virtual const char* factoryType() const { return "player"; }

	// player's unique ID (this is identical to their name)
	inline std::string getId() const { return name.getText(); }

	// name information (special for players)
	inline virtual EntityName getName() const { return name; }

	// description information
	virtual inline std::string getDesc() const { return std::string(); }
	virtual inline void setDesc(const std::string& s_desc) {}

	// gender
	virtual inline GenderType getGender() const { return form.gender; }
	inline void setGender(GenderType gender) { form.gender = gender; }

	// save and load
	virtual void saveData(File::Writer& writer);
	virtual void saveHook(File::Writer& writer);
	void save();

	int loadNode(File::Reader& reader, File::Node& node);

	// output color
	inline const char* ncolor() const { return CPLAYER; }

	// account
	inline std::tr1::shared_ptr<class Account> getAccount() const { return account; }

	// connected?
	//   currently in use by someone
	inline bool isConnected() const { return conn != NULL; }

	// time
	time_t getTimeCreated() const { return time_created; }
	time_t getTimeLastLogin() const { return time_lastlogin; }
	uint32 getTotalPlaytime() const { return total_playtime; }

	// session management
	int startSession();
	void endSession();

	// birthday/age
	uint getAge() const;
	inline GameTime getBirthday() const { return birthday; }
	inline void setBirthday(const GameTime& gt) { birthday = gt; }

	// misc
	void kill(Creature* killer);
	void heartbeat();

	// manage account active counts
	virtual void activate();
	virtual void deactivate();

	// race
	inline class Race* getRace() const { return race; }
	inline class Race* setRace(class Race* race_in) { return race = race_in; }

	// character traits
	FormColor getEyeColor() const { return form.eye_color; }
	FormColor getHairColor() const { return form.hair_color; }
	FormColor getSkinColor() const { return form.skin_color; }
	FormHairStyle getHairStyle() const { return form.hair_style; }
	FormBuild getBuild() const { return form.build; }
	FormHeight getHeight() const { return form.height; }

	void setEyeColor(FormColor value) { form.eye_color = value; }
	void setHairColor(FormColor value) { form.hair_color = value; }
	void setSkinColor(FormColor value) { form.skin_color = value; }
	void setHairStyle(FormHairStyle value) { form.hair_style = value; }
	void setBuild(FormBuild value) { form.build = value; }
	void setHeight(FormHeight value) { form.height = value; }

	// combat
	virtual uint getCombatDodge() const;
	virtual uint getCombatAttack() const;
	virtual uint getCombatDamage() const;

	// class/exp
	inline uint getExp() const { return experience; }
	void grantExp(uint amount);

	// stats
	virtual inline int getBaseStat(CreatureStatID stat) const { return base_stats[stat.getValue()]; }
	inline void setBaseStat(CreatureStatID stat, int value) { base_stats[stat.getValue()] = value; }

	// recalcuate everything
	virtual void recalcStats();
	virtual void recalc();

	// display info
	void displayInventory();
	virtual void displayDesc(const class StreamControl& stream) const;
	void displaySkills();

	// I/O
	virtual void streamPut(const char* data, size_t len = 0);
	void showPrompt();
	void processCommand(const std::string& cmd);
	void connect(IPlayerConnection* conn);
	void disconnect();
	IPlayerConnection* getConn() const { return conn; }
	void toggleEcho(bool value);
	void setIndent(uint level);
	uint getWidth();
	void clearScr();

	// parsing
	virtual int macroProperty(const class StreamControl& stream, const std::string& method, const MacroList& argv) const;

	// player-only actions
	void doTell(Player* who, const std::string& what);
	void doReply(const std::string& what);

protected:
	EntityName name;
	IPlayerConnection* conn;
	std::tr1::shared_ptr<Account> account;
	std::string last_command;
	std::string last_tell;
	struct PDesc {
		GenderType gender;
		FormColor eye_color;
		FormColor hair_color;
		FormColor skin_color;
		FormHairStyle hair_style;
		FormBuild build;
		FormHeight height;
	} form;
	CreatureStatArray base_stats;
	class Race *race;
	uint32 experience;
	time_t time_created;
	time_t time_lastlogin;
	uint32 total_playtime;
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
	bool beginMCCP();
	void endMCCP();
#endif // HAVE_LIBZ

	friend class _MPlayer; // obvious

protected:
	~Player();
};

// manage all players
class _MPlayer : public IManager
{
public:
	// list of *connected* players - do *not* use GC
	typedef std::list<Player*> PlayerList;

	// initialize the player manager
	virtual int initialize();

	// shutdown the player manager
	virtual void shutdown();

	// true if 'name' is a valid player name
	bool validName(const std::string& name);

	// return the path a player's file is at
	std::string path(const std::string& name);

	// return the logged-in player with the given name
	Player* get(const std::string& name);

	// load a player - from disk
	Player* load(std::tr1::shared_ptr<class Account> account, const std::string& name);

	// DESTROY a player permanently (with backup)
	int destroy(const std::string& name);

	// does a valid player of this name exist?
	bool exists(const std::string& name);

	// count of connected players
	size_t count();

	// list all connected players
	void list(const StreamControl& stream);

	// save all players
	void save();

	// EEEEW - return list of all players - yuck
	inline const PlayerList& getPlayerList() { return player_list; }

private:
	PlayerList player_list;

	// yuck - let Player class manage their own membership
	friend class Player;
};
extern _MPlayer MPlayer;

#define PLAYER(ent) E_CAST((ent),Player)

extern std::string getStatLevel(uint value);
extern std::string getStatColor(uint value);

#endif
