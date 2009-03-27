/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_NPC_H
#define SOURCEMUD_MUD_NPC_H

#include "mud/creature.h"
#include "common/imanager.h"

// Npc blueprint
class NpcBP
{
public:
	NpcBP();

	// blueprint id
	inline std::string getId() const { return id; }

	// npc
	inline const std::vector<std::string>& getEquipList() const { return equip_list; }

	// load
	int load(File::Reader& reader);
	void save(File::Writer& writer);

	// parent blueprint
	virtual NpcBP* getParent() const { return parent; }

	// name
	inline const EntityName& getName() const { return name; }
	inline bool setName(const std::string& s_name) { bool ret = name.setFull(s_name); set_flags.name = true; return ret; }
	void resetName();

	inline const std::vector<std::string>& getKeywords() const { return keywords; }

	// description
	inline const std::string& getDesc() const { return desc; }
	inline void setDesc(const std::string& s_desc) { desc = s_desc; set_flags.desc = true; }
	void resetDesc();

	// stats
	inline int getStat(CreatureStatID stat) const { return base_stats[stat]; }
	inline void setStat(CreatureStatID stat, int value) { base_stats[stat] = value; }
	void resetStats();

	// gender
	inline GenderType getGender() const { return gender; }
	inline void setGender(GenderType s_gender) { gender = s_gender; set_flags.gender = true; }
	void resetGender();

	// combat
	inline uint getCombatDodge() const { return combat.dodge; }
	inline uint getCombatAttack() const { return combat.attack; }
	inline uint getCombatDamage() const { return combat.damage; }
	inline void setCombatDodge(uint value) { combat.dodge = value; set_flags.dodge = true; }
	inline void setCombatAttack(uint value) { combat.attack = value; set_flags.attack = true; }
	inline void setCombatDamage(uint value) { combat.damage = value; set_flags.damage = true; }
	void resetCombatDodge();
	void resetCombatAttack();
	void resetCombatDamage();

	// refresh all data
	void refresh();

private:
	std::string id;
	EntityName name;
	std::string desc;
	std::vector<std::string> keywords;
	GenderType gender;
	CreatureStatArray base_stats;
	NpcBP* parent;
	std::vector<std::string> equip_list;

	struct CombatData {
		uint dodge;
		uint attack;
		uint damage;
	} combat;

	struct SetFlags {
		int	name: 1,
			desc: 1,
			gender: 1,
			dodge: 1,
			attack: 1,
			damage: 1,
			stats: 1;

		inline SetFlags() : name(false), desc(false),
				gender(false), dodge(false), attack(false),
				damage(false), stats(false) {}
	} set_flags;

	void setParent(NpcBP* blueprint);
};

class Npc : public Creature
{
public:
	Npc();
	Npc(NpcBP* s_blueprint);

	virtual const char* factoryType() const { return "npc"; }

	// blueprints
	virtual NpcBP* getBlueprint() const { return blueprint; }
	void setBlueprint(NpcBP* s_blueprint);
	static Npc* loadBlueprint(const std::string& name);

	// name info
	virtual EntityName getName() const;

	virtual bool nameMatch(const std::string& name) const;

	// description
	virtual std::string getDesc() const;

	// gender
	virtual GenderType getGender() const;

	// stats
	virtual int getBaseStat(CreatureStatID stat) const;

	// save and load
	virtual int loadNode(File::Reader& reader, File::Node& node);
	virtual int loadFinish();
	virtual void saveData(File::Writer& writer);
	virtual void saveHook(File::Writer& writer);

	// display
	virtual const char* ncolor() const { return CNPC; }

	// return ture if we derive from the named blueprint
	bool isBlueprint(const std::string& blueprint) const;

	// combat
	virtual uint getCombatDodge() const;
	virtual uint getCombatAttack() const;
	virtual uint getCombatDamage() const;

	// movement information
	inline bool isZoneLocked() const { return flags.zonelock; }
	inline void setZoneLocked(bool value) { flags.zonelock = value; }
	inline bool isRoomTagReversed() const { return flags.revroomtag; }
	inline void setRoomTagReversed(bool value) { flags.revroomtag = value; }
	inline TagID getRoomTag() const { return room_tag; }
	inline void setRoomTag(TagID s_room_tag) { room_tag = s_room_tag; }
	bool canUsePortal(class Portal* portal) const;

	// dead
	void kill(Creature* killer);

	// heartbeat
	void heartbeat();

	// handle events
	virtual void handleEvent(const Event& event);

	// display desc
	virtual void displayDesc(const class StreamControl& stream);

protected:
	~Npc();

	void initialize();

	// data
private:
	TagID room_tag; // tag needed in a room to enter it
	NpcBP* blueprint;

	struct NPCFlags {
int zonelock:
		1, // can't leave the zone they're in
revroomtag:
		1; // reverse meaning of room tag
	} flags;
};

class _MNpcBP : public IManager
{
	typedef std::map<std::string, NpcBP*> BlueprintMap;

public:
	int initialize();
	void shutdown();

	NpcBP* lookup(const std::string& id);

private:
	BlueprintMap blueprints;
};

extern _MNpcBP MNpcBP;

#define NPC(ent) E_CAST((ent),Npc)

#endif
