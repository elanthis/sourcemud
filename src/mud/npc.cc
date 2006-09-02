/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <unistd.h>
#include <dirent.h>

#include "mud/body.h"
#include "mud/npc.h"
#include "mud/room.h"
#include "common/streams.h"
#include "mud/parse.h"
#include "mud/eventids.h"
#include "mud/settings.h"
#include "mud/object.h"
#include "mud/skill.h"
#include "mud/hooks.h"
#include "mud/bindings.h"

void
NpcBlueprint::reset_name (void)
{
	// clear
	name.set_name(S("an npc"));
	set_flags.name = false;

	// get parent value
	const NpcBlueprint* data = get_parent();
	if (data != NULL) {
		name = data->get_name();
	}
}

void
NpcBlueprint::reset_desc (void)
{
	// clear
	desc = String("npc");
	set_flags.desc = false;

	// get parent value
	const NpcBlueprint* data = get_parent();
	if (data != NULL)
		desc = data->get_desc();
}

void
NpcBlueprint::reset_gender (void)
{
	// reset
	gender = GenderType::NONE;
	set_flags.gender = false;

	// get parent value
	const NpcBlueprint* data = get_parent();
	if (data != NULL)
		gender = data->get_gender();
}

void
NpcBlueprint::reset_alignment (void)
{
	// reset
	alignment = 0;
	set_flags.alignment = false;

	// get parent value
	const NpcBlueprint* data = get_parent();
	if (data != NULL)
		alignment = data->get_alignment();
}

void
NpcBlueprint::reset_combat_dodge (void)
{
	// reset
	combat.dodge = 0;
	set_flags.dodge = false;

	// get parent
	const NpcBlueprint* data = get_parent ();
	if (data != NULL)
		combat.dodge = data->get_combat_dodge();
}

void
NpcBlueprint::reset_combat_attack (void)
{
	// reset
	combat.attack = 0;
	set_flags.attack = false;

	// get parent
	const NpcBlueprint* data = get_parent ();
	if (data != NULL)
		combat.attack = data->get_combat_attack();
}

void
NpcBlueprint::reset_combat_damage (void)
{
	// reset
	combat.damage = 0;
	set_flags.damage = false;

	// get parent
	const NpcBlueprint* data = get_parent ();
	if (data != NULL)
		combat.damage = data->get_combat_damage();
}

void
NpcBlueprint::reset_stats (void)
{
	// reset
	for (int i = 0; i < CharStatID::COUNT; ++i)
		base_stats[i] = 0;
	set_flags.stats = false;

	// get parent
	const NpcBlueprint* data = get_parent ();
	if (data != NULL)
		for (int i = 0; i < CharStatID::COUNT; ++i)
			base_stats[i] = data->get_stat(i);
}

void
NpcBlueprint::refresh (void)
{
	if (!set_flags.name)
		reset_name();
	if (!set_flags.desc)
		reset_desc();
	if (!set_flags.gender)
		reset_gender();
	if (!set_flags.alignment)
		reset_alignment();
	if (!set_flags.dodge)
		reset_combat_dodge();
	if (!set_flags.attack)
		reset_combat_attack();
	if (!set_flags.damage)
		reset_combat_damage();
	if (!set_flags.stats)
		reset_stats();
}

// ----- NpcBlueprint -----

SCRIPT_TYPE(NPCBlueprint);
NpcBlueprint::NpcBlueprint (void) : Scriptix::Native(AweMUD_NPCBlueprintType), parent(NULL) {}

void
NpcBlueprint::set_parent (NpcBlueprint* blueprint)
{
	parent = blueprint;
	refresh();
}

int
NpcBlueprint::load (File::Reader& reader)
{
	FO_READ_BEGIN
		FO_ATTR2("blueprint", "id")
			id = node.get_data();
		FO_ATTR2("blueprint", "ai")
			ai = AIManager.get(node.get_data());
			if (ai == NULL)
				Log::Warning << "Unknown AI system '" << node.get_data() << "' at " << reader.get_filename() << ':' << node.get_line();
		FO_ATTR2("blueprint", "parent")
			NpcBlueprint* blueprint = NpcBlueprintManager.lookup(node.get_data());
			if (blueprint)
				set_parent(blueprint);
			else
				Log::Warning << "Undefined parent npc blueprint '" << node.get_data() << "' at " << reader.get_filename() << ':' << node.get_line();
		FO_ATTR2("blueprint", "equip")
			equip_list.push_back(node.get_data());
		FO_KEYED("user")
			if (node.get_datatype() == File::TYPE_INT)
				set_property(node.get_key(), tolong(node.get_data()));
			else if (node.get_datatype() == File::TYPE_STRING)
				set_property(node.get_key(), node.get_data());
			else if (node.get_datatype() == File::TYPE_BOOL)
				set_property(node.get_key(), str_is_true(node.get_data()));
			else {
				Log::Error << "Invalid data type for script attribute at " << reader.get_filename() << ':' << node.get_line();
				return -1;
			}
		FO_ATTR2("blueprint", "name")
			set_name(node.get_data());
		FO_ATTR2("blueprint", "keyword")
			keywords.push_back(node.get_data());
		FO_ATTR2("blueprint", "desc")
			set_desc(node.get_data());
		FO_ATTR2("blueprint", "gender")
			set_gender(GenderType::lookup(node.get_data()));
		FO_ATTR2("blueprint", "alignment")
			FO_TYPE_ASSERT(INT);
			set_alignment(tolong(node.get_data()));
		FO_ATTR2("combat", "dodge")
			FO_TYPE_ASSERT(INT);
			combat.dodge = tolong(node.get_data());
			set_flags.dodge = true;
		FO_ATTR2("combat", "attack")
			FO_TYPE_ASSERT(INT);
			combat.attack = tolong(node.get_data());
			set_flags.attack = true;
		FO_ATTR2("combat", "damage")
			FO_TYPE_ASSERT(INT);
			combat.damage = tolong(node.get_data());
			set_flags.damage = true;
		FO_KEYED("stat")
			CharStatID stat = CharStatID::lookup(node.get_key());
			if (stat) {
				FO_TYPE_ASSERT(INT);
				base_stats[stat.get_value()] = tolong(node.get_data());
			}
	FO_READ_ERROR
		return -1;
	FO_READ_END

	return 0;
}

void
NpcBlueprint::save (File::Writer& writer)
{
	if (set_flags.name)
		writer.keyed(S("blueprint"), S("name"), name.get_name());

	for (StringList::const_iterator i = keywords.begin(); i != keywords.end(); ++i)
		writer.keyed(S("blueprint"), S("keyword"), *i);

	if (set_flags.desc)
		writer.keyed(S("blueprint"), S("desc"), desc);

	if (set_flags.alignment)
		writer.keyed(S("blueprint"), S("alignment"), alignment);

	if (set_flags.gender)
		writer.keyed(S("blueprint"), S("gender"), gender.get_name());

	if (set_flags.stats)
		for (int i = 0; i < CharStatID::COUNT; ++i)
			writer.keyed(S("stat"), CharStatID(i).get_name(), base_stats[i]);

	if (set_flags.dodge)
		writer.keyed(S("combat"), S("dodge"), combat.dodge);
	if (set_flags.attack)
		writer.keyed(S("combat"), S("attack"), combat.attack);
	if (set_flags.damage)
		writer.keyed(S("combat"), S("damage"), combat.damage);
}

Scriptix::Value
NpcBlueprint::get_undefined_property (Scriptix::Atom id) const
{
	if (parent == NULL)
		return Scriptix::Nil;
	return parent->get_property(id);
}

// ----- Npc -----

SCRIPT_TYPE(NPC);
Npc::Npc (void) : Character (AweMUD_NPCType)
{
	initialize();
}

Npc::Npc (NpcBlueprint* s_blueprint) : Character (AweMUD_NPCType)
{
	initialize();
	blueprint = NULL;
	set_blueprint(s_blueprint);
}

void
Npc::initialize (void)
{
	ai = NULL;
	blueprint = NULL;
	flags.zonelock = false;
	flags.revroomtag = false;
	room_tag = TagID();
}

Npc::~Npc (void)
{
}

int
Npc::load_finish (void)
{
	if (Character::load_finish())
		return -1;
	
	if (blueprint == NULL) {
		Log::Error << "NPC has no blueprint";
		return -1;
	}
		
	return 0;
}

int
Npc::load_node (File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_ATTR2("npc", "blueprint")
			NpcBlueprint* blueprint;
			if ((blueprint = NpcBlueprintManager.lookup(node.get_data())) == NULL)
				Log::Error << "Could not find npc blueprint '" << node.get_data() << "'";
			else
				set_blueprint(blueprint);
		FO_ATTR2("npc", "ai")
			ai = AIManager.get(node.get_data());
			if (ai == NULL)
				Log::Error << "Unknown AI system '" << node.get_data() << "' at " << reader.get_filename() << ':' << node.get_line();
		FO_ATTR2("npc", "roomtag")
			room_tag = TagID::create(node.get_data());
		FO_ATTR2("npc", "zonelock")
			FO_TYPE_ASSERT(BOOL);
			flags.zonelock = str_is_true(node.get_data());
		FO_ATTR2("npc", "reverse_roomtag")
			FO_TYPE_ASSERT(BOOL);
			flags.revroomtag = str_is_true(node.get_data());
		FO_PARENT(Character)
	FO_NODE_END
}

void
Npc::save (File::Writer& writer)
{
	if (get_blueprint())
		writer.keyed(S("npc"), S("blueprint"), get_blueprint()->get_id());

	Character::save(writer);

	if (ai != NULL)
		writer.keyed(S("npc"), S("ai"), ai->get_name());

	if (room_tag.valid())
		writer.keyed(S("npc"), S("roomtag"), TagID::nameof(room_tag));
	if (flags.zonelock)
		writer.keyed(S("npc"), S("zonelock"), true);
	if (flags.revroomtag)
		writer.keyed(S("npc"), S("reverse_roomtag"), true);
}

void
Npc::save_hook (ScriptRestrictedWriter* writer)
{
	Character::save_hook(writer);
	Hooks::save_npc(this, writer);

	if (ai != NULL)
		ai->do_save(this, writer);
}
	
EntityName
Npc::get_name (void) const
{
	assert(blueprint != NULL);
	return blueprint->get_name();
}

String
Npc::get_desc (void) const
{
	assert(blueprint != NULL);
	return blueprint->get_desc();
}

GenderType
Npc::get_gender (void) const
{
	assert(blueprint != NULL);
	return blueprint->get_gender();
}

int
Npc::get_base_stat (CharStatID stat) const
{
	assert(blueprint != NULL);
	return blueprint->get_stat(stat);
}

CharAlign
Npc::get_alignment (void) const
{
	assert(blueprint != NULL);
	return blueprint->get_alignment();
}

uint
Npc::get_combat_dodge (void) const
{
	assert(blueprint != NULL);
	return blueprint->get_combat_dodge();
}

uint
Npc::get_combat_attack (void) const
{
	assert(blueprint != NULL);
	return blueprint->get_combat_attack();
}

uint
Npc::get_combat_damage (void) const
{
	assert(blueprint != NULL);
	return blueprint->get_combat_damage();
}

AI*
Npc::get_ai (void) const
{
	// we have it?
	if (ai)
		return ai;

	// blueprints
	NpcBlueprint* blueprint = get_blueprint();
	while (blueprint != NULL) {
		if (blueprint->get_ai())
			return blueprint->get_ai();
		blueprint = blueprint->get_parent();
	}

	return NULL;
}

void
Npc::pump (Scriptix::Value data)
{
	AI* ai = get_ai();
	if (ai)
		ai->do_pump(this, data);
}

void
Npc::kill (Character *killer)
{
	// death message
	if (get_room())
		*get_room() << StreamName(this, DEFINITE, true) << " has been slain!\n";

	// lay down, drop crap
	position = CharPosition::LAY;

	// hook/event
	Events::send_death(get_room(), this, killer);
	if (!Hooks::npc_death(this, killer)) {
		// only if there's no hook - hooks must do this for us!
		destroy();
	}
}

int
Npc::handle_event (const Event& event)
{
	// ai
	AI* ai = get_ai();
	if (ai)
		ai->do_event (this, event);

	// normal event handler
	return Entity::handle_event (event);
}

void
Npc::heartbeat (void)
{
	// have rt and ai?
	bool have_rt = get_round_time() > 0;

	// do character update
	Character::heartbeat();

	// ai heartbeart
	AI* ai = get_ai();
	if (ai)
		ai->do_heartbeat(this);

	// round time ended? do AI
	if (ai && have_rt && get_round_time() == 0)
		ai->do_ready(this);

	// update handler
	Hooks::npc_heartbeat(this);
}

void
Npc::set_blueprint (NpcBlueprint* s_blueprint)
{
	blueprint = s_blueprint;
	for (int i = 0; i < CharStatID::COUNT; ++i)
		Character::set_effective_stat(CharStatID(i), get_base_stat(CharStatID(i)));
	Character::recalc();
}

// load npc from a blueprint
Npc*
Npc::load_blueprint (String name)
{
	// lookup the blueprint
	NpcBlueprint* blueprint = NpcBlueprintManager.lookup(name);
	if (!blueprint)
		return NULL;
	
	// create it
	Npc* npc = new Npc(blueprint);
	if (npc == NULL)
		return NULL;

	// equip
	while (blueprint != NULL) {
		for (StringList::const_iterator i = blueprint->get_equip_list().begin(); i != blueprint->get_equip_list().end(); ++i) {
			Object* object = Object::load_blueprint(*i);
			if (object != NULL)
				npc->equip(object);
			else
				Log::Error << "Object blueprint '" << *i << "' from NPC blueprint '" << blueprint->get_id() << "' does not exist.";
		}
		blueprint = blueprint->get_parent();
	}

	// set HP
	npc->set_hp(npc->get_max_hp());

	return npc;
}

// display NPC description
void
Npc::display_desc (const StreamControl& stream)
{
	if (get_desc ())
		stream << StreamParse(get_desc(), S("npc"), this); // FIXME: re-enable 'actor'(looker)
	else
		stream << StreamName(this, DEFINITE, true) << " doesn't appear very interesting.";
}

bool
Npc::can_use_exit (RoomExit* exit) const
{
	assert(exit != NULL);

	// disabled exits can't be used
	if (exit->is_disabled())
		return false;

	// exit's room is our room, yes?
	if (get_room() != exit->get_room())
		return false;

	// get target room; must be valid
	Room* troom = exit->get_target_room();
	if (troom == NULL)
		return false;

	// check zone constraints
	if (is_zone_locked() && troom->get_zone() != get_room()->get_zone())
		return false;

	// check room tag constraints
	if (room_tag.valid()) {
		// does the room have the tag?
		if (troom->has_tag(room_tag)) {
			// reversed?
			if (is_room_tag_reversed())
				return false;
		// does not have tag
		} else {
			// required it?
			if (!is_room_tag_reversed())
				return false;
		}
	}

	// FIXME: check exit usage types; climb, crawl, etc

	// no failures - all good
	return true;
}

bool
Npc::is_blueprint (String name) const
{
	NpcBlueprint* blueprint = get_blueprint();

	while (blueprint != NULL) {
		if (str_eq(blueprint->get_id(), name))
			return true;

		blueprint = blueprint->get_parent();
	}

	return false;
}

bool
Npc::name_match (String match) const
{
	if (get_name().matches(match))
		return true;

	// blueprint keywords
	NpcBlueprint* blueprint = get_blueprint();
	while (blueprint != NULL) {
		for (StringList::const_iterator i = blueprint->get_keywords().begin(); i != blueprint->get_keywords().end(); i ++)
			if (phrase_match (*i, match))
				return true;

		blueprint = blueprint->get_parent();
	}

	// no match
	return false;
}

Scriptix::Value
Npc::get_undefined_property (Scriptix::Atom id) const
{
	const NpcBlueprint* data = get_blueprint();
	if (data == NULL)
		return Scriptix::Nil;
	return data->get_property(id);
}

// Npc Blueprint Manager

SNpcBlueprintManager NpcBlueprintManager;

int
SNpcBlueprintManager::initialize (void)
{
	// requirements
	if (require(SkillManager) != 0)
		return 1;
	if (require(AIManager) != 0)
		return 1;
	if (require(ObjectBlueprintManager) != 0)
		return 1;
	if (require(ScriptBindings) != 0)
		return 1;
	if (require(EventManager) != 0)
		return 1;

	String path = SettingsManager.get_blueprint_path();
	
	dirent* d_ent;
	DIR* dir = opendir(path.c_str());
	if (!dir) {
		Log::Error << "Failed to open blueprint folder '" << path << "': " << strerror(errno);
		return -1;
	}
	while ((d_ent = readdir(dir)) != NULL) {
		// match file name
		size_t len = strlen(d_ent->d_name);
		if (len >= 6 && d_ent->d_name[0] != '.' && !strcmp(".npcs", &d_ent->d_name[len - 5])) {
			// load from file
			File::Reader reader;
			if (reader.open(path + S("/") + String(d_ent->d_name)))
				return -1;
			FO_READ_BEGIN
				FO_OBJECT("npc_blueprint")
					NpcBlueprint* blueprint = new NpcBlueprint();
					if (blueprint->load(reader)) {
						Log::Warning << "Failed to load blueprint in " << reader.get_filename() << " at " << node.get_line();
						return -1;
					}

					if (!blueprint->get_id()) {
						Log::Warning << "Blueprint has no ID in " << reader.get_filename() << " at " << node.get_line();
						return -1;
					}

					blueprints[blueprint->get_id()] = blueprint;
			FO_READ_ERROR
				return -1;
			FO_READ_END
		}
	}
	closedir(dir);

	return 0;
}

void
SNpcBlueprintManager::shutdown (void)
{
}

NpcBlueprint*
SNpcBlueprintManager::lookup (String id)
{
	BlueprintMap::iterator iter = blueprints.find(id);
	if (iter == blueprints.end())
		return NULL;
	else
		return iter->second;
}
