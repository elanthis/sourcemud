/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include <unistd.h>

#include "mud/body.h"
#include "mud/npc.h"
#include "mud/room.h"
#include "common/streams.h"
#include "mud/macro.h"
#include "mud/settings.h"
#include "mud/object.h"
#include "mud/skill.h"
#include "mud/hooks.h"
#include "common/manifest.h"
#include "mud/shadow-object.h"
#include "mud/efactory.h"

void
NpcBP::reset_name()
{
	// clear
	name.set_name(S("an npc"));
	set_flags.name = false;

	// get parent value
	const NpcBP* data = get_parent();
	if (data != NULL) {
		name = data->get_name();
	}
}

void
NpcBP::reset_desc()
{
	// clear
	desc = String("npc");
	set_flags.desc = false;

	// get parent value
	const NpcBP* data = get_parent();
	if (data != NULL)
		desc = data->get_desc();
}

void
NpcBP::reset_gender()
{
	// reset
	gender = GenderType::NONE;
	set_flags.gender = false;

	// get parent value
	const NpcBP* data = get_parent();
	if (data != NULL)
		gender = data->get_gender();
}

void
NpcBP::reset_combat_dodge()
{
	// reset
	combat.dodge = 0;
	set_flags.dodge = false;

	// get parent
	const NpcBP* data = get_parent();
	if (data != NULL)
		combat.dodge = data->get_combat_dodge();
}

void
NpcBP::reset_combat_attack()
{
	// reset
	combat.attack = 0;
	set_flags.attack = false;

	// get parent
	const NpcBP* data = get_parent();
	if (data != NULL)
		combat.attack = data->get_combat_attack();
}

void
NpcBP::reset_combat_damage()
{
	// reset
	combat.damage = 0;
	set_flags.damage = false;

	// get parent
	const NpcBP* data = get_parent();
	if (data != NULL)
		combat.damage = data->get_combat_damage();
}

void
NpcBP::reset_stats()
{
	// reset
	for (int i = 0; i < CreatureStatID::COUNT; ++i)
		base_stats[i] = 0;
	set_flags.stats = false;

	// get parent
	const NpcBP* data = get_parent();
	if (data != NULL)
		for (int i = 0; i < CreatureStatID::COUNT; ++i)
			base_stats[i] = data->get_stat(i);
}

void
NpcBP::refresh()
{
	if (!set_flags.name)
		reset_name();
	if (!set_flags.desc)
		reset_desc();
	if (!set_flags.gender)
		reset_gender();
	if (!set_flags.dodge)
		reset_combat_dodge();
	if (!set_flags.attack)
		reset_combat_attack();
	if (!set_flags.damage)
		reset_combat_damage();
	if (!set_flags.stats)
		reset_stats();
}

// ----- NpcBP -----

NpcBP::NpcBP() : parent(NULL) {}

void
NpcBP::set_parent(NpcBP* blueprint)
{
	parent = blueprint;
	refresh();
}

int
NpcBP::load(File::Reader& reader)
{
	FO_READ_BEGIN
		FO_ATTR("blueprint", "id")
			id = node.get_string();
		FO_ATTR("blueprint", "ai")
			ai = AIManager.get(node.get_string());
			if (ai == NULL)
				Log::Warning << "Unknown AI system '" << node.get_string() << "' at " << reader.get_filename() << ':' << node.get_line();
		FO_ATTR("blueprint", "parent")
			NpcBP* blueprint = NpcBPManager.lookup(node.get_string());
			if (blueprint)
				set_parent(blueprint);
			else
				Log::Warning << "Undefined parent npc blueprint '" << node.get_string() << "' at " << reader.get_filename() << ':' << node.get_line();
		FO_ATTR("blueprint", "equip")
			equip_list.push_back(node.get_string());
		FO_ATTR("blueprint", "name")
			set_name(node.get_string());
		FO_ATTR("blueprint", "keyword")
			keywords.push_back(node.get_string());
		FO_ATTR("blueprint", "desc")
			set_desc(node.get_string());
		FO_ATTR("blueprint", "gender")
			set_gender(GenderType::lookup(node.get_string()));
		FO_ATTR("combat", "dodge")
			combat.dodge = node.get_int();
			set_flags.dodge = true;
		FO_ATTR("combat", "attack")
			combat.attack = node.get_int();
			set_flags.attack = true;
		FO_ATTR("combat", "damage")
			combat.damage = node.get_int();
			set_flags.damage = true;
		FO_ATTR("blueprint", "stat")
			CreatureStatID stat = CreatureStatID::lookup(node.get_string(0));
			if (stat) {
				base_stats[stat.get_value()] = node.get_int(1);
			}
	FO_READ_ERROR
		return -1;
	FO_READ_END

	return 0;
}

void
NpcBP::save(File::Writer& writer)
{
	if (set_flags.name)
		writer.attr(S("blueprint"), S("name"), name.get_name());

	for (StringList::const_iterator i = keywords.begin(); i != keywords.end(); ++i)
		writer.attr(S("blueprint"), S("keyword"), *i);

	if (set_flags.desc)
		writer.attr(S("blueprint"), S("desc"), desc);

	if (set_flags.gender)
		writer.attr(S("blueprint"), S("gender"), gender.get_name());

	if (set_flags.stats)
		for (int i = 0; i < CreatureStatID::COUNT; ++i)
			writer.attr(S("stat"), CreatureStatID(i).get_name(), base_stats[i]);

	if (set_flags.dodge)
		writer.attr(S("combat"), S("dodge"), combat.dodge);
	if (set_flags.attack)
		writer.attr(S("combat"), S("attack"), combat.attack);
	if (set_flags.damage)
		writer.attr(S("combat"), S("damage"), combat.damage);
}

// ----- Npc -----

Npc::Npc() : Creature()
{
	initialize();
}

Npc::Npc (NpcBP* s_blueprint) : Creature()
{
	initialize();
	blueprint = NULL;
	set_blueprint(s_blueprint);
}

void
Npc::initialize()
{
	ai = NULL;
	blueprint = NULL;
	flags.zonelock = false;
	flags.revroomtag = false;
	room_tag = TagID();
}

Npc::~Npc()
{
}

int
Npc::load_finish()
{
	if (Creature::load_finish())
		return -1;
	
	if (blueprint == NULL) {
		Log::Error << "NPC has no blueprint";
		return -1;
	}
		
	return 0;
}

int
Npc::load_node(File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_ATTR("npc", "blueprint")
			NpcBP* blueprint;
			if ((blueprint = NpcBPManager.lookup(node.get_string())) == NULL)
				Log::Error << "Could not find npc blueprint '" << node.get_string() << "'";
			else
				set_blueprint(blueprint);
		FO_ATTR("npc", "ai")
			ai = AIManager.get(node.get_string());
			if (ai == NULL)
				Log::Error << "Unknown AI system '" << node.get_string() << "' at " << reader.get_filename() << ':' << node.get_line();
		FO_ATTR("npc", "roomtag")
			room_tag = TagID::create(node.get_string());
		FO_ATTR("npc", "zonelock")
			flags.zonelock = node.get_bool();
		FO_ATTR("npc", "reverse_roomtag")
			flags.revroomtag = node.get_bool();
		FO_PARENT(Creature)
	FO_NODE_END
}

void
Npc::save_data(File::Writer& writer)
{
	if (get_blueprint())
		writer.attr(S("npc"), S("blueprint"), get_blueprint()->get_id());

	Creature::save_data(writer);

	if (ai != NULL)
		writer.attr(S("npc"), S("ai"), ai->get_name());

	if (room_tag.valid())
		writer.attr(S("npc"), S("roomtag"), TagID::nameof(room_tag));
	if (flags.zonelock)
		writer.attr(S("npc"), S("zonelock"), true);
	if (flags.revroomtag)
		writer.attr(S("npc"), S("reverse_roomtag"), true);
}

void
Npc::save_hook(File::Writer& writer)
{
	Creature::save_hook(writer);
	Hooks::save_npc(this, writer);

	if (ai != NULL)
		ai->do_save(this, writer);
}
	
EntityName
Npc::get_name() const
{
	assert(blueprint != NULL);
	return blueprint->get_name();
}

String
Npc::get_desc() const
{
	assert(blueprint != NULL);
	return blueprint->get_desc();
}

GenderType
Npc::get_gender() const
{
	assert(blueprint != NULL);
	return blueprint->get_gender();
}

int
Npc::get_base_stat(CreatureStatID stat) const
{
	assert(blueprint != NULL);
	return blueprint->get_stat(stat);
}

uint
Npc::get_combat_dodge() const
{
	assert(blueprint != NULL);
	return blueprint->get_combat_dodge();
}

uint
Npc::get_combat_attack() const
{
	assert(blueprint != NULL);
	return blueprint->get_combat_attack();
}

uint
Npc::get_combat_damage() const
{
	assert(blueprint != NULL);
	return blueprint->get_combat_damage();
}

AI*
Npc::get_ai() const
{
	// we have it?
	if (ai)
		return ai;

	// blueprints
	NpcBP* blueprint = get_blueprint();
	while (blueprint != NULL) {
		if (blueprint->get_ai())
			return blueprint->get_ai();
		blueprint = blueprint->get_parent();
	}

	return NULL;
}

void
Npc::pump()
{
	AI* ai = get_ai();
	if (ai)
		ai->do_pump(this);
}

void
Npc::kill(Creature *killer)
{
	// death message
	if (get_room())
		*get_room() << StreamName(this, DEFINITE, true) << " has been slain!\n";

	// lay down, drop crap
	position = CreaturePosition::LAY;

	// FIXME EVENT
	if (!Hooks::npc_death(this, killer)) {
		// only if there's no hook - hooks must do this for us!
		destroy();
	}
}

void
Npc::handle_event(const Event& event)
{
	// ai
	AI* ai = get_ai();
	if (ai)
		ai->do_event(this, event);

	// normal event handler
	Entity::handle_event(event);
}

void
Npc::heartbeat()
{
	// have rt and ai?
	bool have_rt = get_round_time() > 0;

	// do creature update
	Creature::heartbeat();

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
Npc::set_blueprint(NpcBP* s_blueprint)
{
	blueprint = s_blueprint;
	for (int i = 0; i < CreatureStatID::COUNT; ++i)
		Creature::set_effective_stat(CreatureStatID(i), get_base_stat(CreatureStatID(i)));
	Creature::recalc();
}

// load npc from a blueprint
Npc*
Npc::load_blueprint(String name)
{
	// lookup the blueprint
	NpcBP* blueprint = NpcBPManager.lookup(name);
	if (!blueprint)
		return NULL;
	
	// create it
	Npc* npc = new Npc(blueprint);
	if (npc == NULL)
		return NULL;

	// equip
	while (blueprint != NULL) {
		for (StringList::const_iterator i = blueprint->get_equip_list().begin(); i != blueprint->get_equip_list().end(); ++i) {
			Object* object = ShadowObject::load_blueprint(*i);
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
Npc::display_desc(const StreamControl& stream)
{
	if (get_desc())
		stream << StreamMacro(get_desc(), S("npc"), this); // FIXME: re-enable 'actor'(looker)
	else
		stream << StreamName(this, DEFINITE, true) << " doesn't appear very interesting.";
}

bool
Npc::can_use_portal(Portal* portal) const
{
	assert(portal != NULL);

	// disabled portals can't be used
	if (portal->is_disabled())
		return false;

	// portal's room is our room, yes?
	if (!portal->has_room(get_room()))
		return false;

	// get target room; must be valid
	Room* troom = portal->get_relative_target(get_room());
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

	// FIXME: check portal usage types; climb, crawl, etc

	// no failures - all good
	return true;
}

bool
Npc::is_blueprint(String name) const
{
	NpcBP* blueprint = get_blueprint();

	while (blueprint != NULL) {
		if (str_eq(blueprint->get_id(), name))
			return true;

		blueprint = blueprint->get_parent();
	}

	return false;
}

bool
Npc::name_match(String match) const
{
	if (get_name().matches(match))
		return true;

	// blueprint keywords
	NpcBP* blueprint = get_blueprint();
	while (blueprint != NULL) {
		for (StringList::const_iterator i = blueprint->get_keywords().begin(); i != blueprint->get_keywords().end(); i ++)
			if (phrase_match (*i, match))
				return true;

		blueprint = blueprint->get_parent();
	}

	// no match
	return false;
}

// Npc Blueprint Manager

SNpcBPManager NpcBPManager;

int
SNpcBPManager::initialize()
{
	// requirements
	if (require(AIManager) != 0)
		return 1;
	if (require(ObjectBPManager) != 0)
		return 1;
	if (require(EventManager) != 0)
		return 1;

	ManifestFile man(SettingsManager.get_blueprint_path(), S(".npcs"));
	StringList files = man.get_files();;
	for (StringList::iterator i = files.begin(); i != files.end(); ++i) {
		File::Reader reader;
		if (reader.open(*i))
			return -1;
		FO_READ_BEGIN
			FO_OBJECT("blueprint", "npc")
				NpcBP* blueprint = new NpcBP();
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

	return 0;
}

void
SNpcBPManager::shutdown()
{
	for (BlueprintMap::iterator i = blueprints.begin(), e = blueprints.end();
			i != e; ++i)
		delete i->second;
}

NpcBP*
SNpcBPManager::lookup(String id)
{
	BlueprintMap::iterator iter = blueprints.find(id);
	if (iter == blueprints.end())
		return NULL;
	else
		return iter->second;
}

BEGIN_EFACTORY(NPC)
	return new Npc();
END_EFACTORY
