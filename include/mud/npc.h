/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef NPC_H
#define NPC_H

#include "char.h"
#include "ai.h"
#include "imanager.h"

// Npc blueprint
class
NpcBlueprint : public Scriptix::Native
{
	public:
	NpcBlueprint (void);

	// blueprint id
	inline String get_id (void) const { return id; }

	// npc
	inline AI* get_ai (void) const { return ai; }
	inline const StringList& get_equip_list (void) const { return equip_list; }

	// load
	int load (File::Reader& reader);
	void save (File::Writer& writer);

	// parent blueprint
	virtual NpcBlueprint* get_parent(void) const { return parent; }

	// name
	inline const String& get_name (void) const { return name.name; }
	inline void set_name (StringArg s_name) { name.name = s_name; set_flags.name = true; }
	inline EntityArticle get_article (void) const { return name.article; }
	inline void set_article (EntityArticle s_article) { name.article = s_article; set_flags.article = true; }
	void reset_name (void);

	inline const StringList& get_keywords (void) const { return keywords; }

	// description
	inline const String& get_desc (void) const { return desc; }
	inline void set_desc (StringArg s_desc) { desc = s_desc; set_flags.desc = true; }
	void reset_desc (void);

	// stats
	inline int get_stat (CharStatID stat) const { return base_stats[stat]; }
	inline void set_stat (CharStatID stat, int value) { base_stats[stat] = value; }
	void reset_stats (void);

	// gender
	inline GenderType get_gender (void) const { return gender; }
	inline void set_gender (GenderType s_gender) { gender = s_gender; set_flags.gender = true; }
	void reset_gender (void);

	// alignment
	inline CharAlign get_alignment (void) const { return alignment; }
	inline void set_alignment (CharAlign s_align) { alignment = s_align; set_flags.alignment = true; }
	void reset_alignment (void);

	// combat
	inline uint get_combat_dodge (void) const { return combat.dodge; }
	inline uint get_combat_attack (void) const { return combat.attack; }
	inline uint get_combat_damage (void) const { return combat.damage; }
	inline void set_combat_dodge (uint value) { combat.dodge = value; set_flags.dodge = true; }
	inline void set_combat_attack (uint value) { combat.attack = value; set_flags.attack = true; }
	inline void set_combat_damage (uint value) { combat.damage = value; set_flags.damage = true; }
	void reset_combat_dodge (void);
	void reset_combat_attack (void);
	void reset_combat_damage (void);

	// refresh all data
	void refresh (void);

	private:
	String id;
	EntityName name;
	String desc;
	StringList keywords;
	GenderType gender;
	CharAlign alignment;
	CharStatArray base_stats;
	NpcBlueprint* parent;
	StringList equip_list;
	AI* ai;

	struct CombatData {
		uint dodge;
		uint attack;
		uint damage;
	} combat;

	struct SetFlags {
		int	name:1,
			article:1,
			desc:1,
			gender:1,
			alignment:1,
			dodge:1,
			attack:1,
			damage:1,
			stats:1;
		inline SetFlags (void) : name(false), article(false), desc(false),
			gender(false), alignment(false), dodge(false), attack(false),
			damage(false), stats(false) {}
	} set_flags;

	virtual Scriptix::Value get_undefined_property (Scriptix::Atom id) const;

	void set_parent (NpcBlueprint* blueprint);
};

class Npc : public Character
{
	public:
	Npc (void);
	Npc (NpcBlueprint* s_blueprint);

	// blueprints
	virtual NpcBlueprint* get_blueprint (void) const { return blueprint; }
	void set_blueprint (NpcBlueprint* s_blueprint);
	static Npc* load_blueprint (StringArg name);

	// name info
	virtual String get_name (void) const;
	virtual EntityArticle get_article (void) const;

	virtual bool name_match (StringArg name) const;

	// description
	virtual String get_desc (void) const;

	// not editable
	virtual void set_name (StringArg s_name) {}
	virtual void set_desc (StringArg s_desc) {}
	virtual void set_article (EntityArticle s_type) {}

	// gender
	virtual GenderType get_gender (void) const;

	// stats
	virtual int get_base_stat (CharStatID stat) const;

	// alignment
	virtual CharAlign get_alignment (void) const;

	// save and load
	virtual int load_node (File::Reader& reader, File::Node& node);
	virtual int load_finish (void);
	virtual void save (File::Writer& writer);
	virtual void save_hook (class ScriptRestrictedWriter* writer);

	// display
	virtual const char *ncolor (void) const { return CNPC; }

	// return ture if we derive from the named blueprint
	bool is_blueprint (StringArg blueprint) const;

	// combat
	virtual uint get_combat_dodge (void) const;
	virtual uint get_combat_attack (void) const;
	virtual uint get_combat_damage (void) const;

	// movement information
	inline bool is_zone_locked (void) const { return flags.zonelock; }
	inline void set_zone_locked (bool value) { flags.zonelock = value; }
	inline bool is_room_tag_reversed (void) const { return flags.revroomtag; }
	inline void set_room_tag_reversed (bool value) { flags.revroomtag = value; }
	inline TagID get_room_tag (void) const { return room_tag; }
	inline void set_room_tag (TagID s_room_tag) { room_tag = s_room_tag; }
	bool can_use_exit (class RoomExit* exit) const;

	// Manage AI
	AI* get_ai (void) const;
	void pump (Scriptix::Value arg = NULL); // call a generic 'pump' event in the AI

	// dead
	void kill (Character* killer);

	// heartbeat
	void heartbeat (void);

	// handle vents - AI
	virtual int handle_event (const Event& event);

	// display desc
	virtual void display_desc (const class StreamControl& stream);

	protected:
	~Npc (void);

	void initialize (void);

	// data
	private:
	TagID room_tag; // tag needed in a room to enter it
	NpcBlueprint* blueprint;
	AI* ai;

	struct NPCFlags {
		int zonelock:1, // can't leave the zone they're in
		revroomtag:1; // reverse meaning of room tag
	} flags;

	protected:
	virtual Scriptix::Value get_undefined_property (Scriptix::Atom id) const;

	E_SUBTYPE(Npc, Character);
};

class SNpcBlueprintManager : public IManager
{
	typedef GCType::map<String,NpcBlueprint*> BlueprintMap;

	public:
	int initialize (void);

	void shutdown (void);

	NpcBlueprint* lookup (StringArg id);

	private:
	BlueprintMap blueprints;
};

extern SNpcBlueprintManager NpcBlueprintManager;

#define NPC(ent) E_CAST((ent),Npc)

#endif
