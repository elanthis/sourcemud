/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/file.h"
#include "common/streams.h"
#include "common/string.h"
#include "mud/body.h"
#include "mud/npc.h"
#include "mud/room.h"
#include "mud/macro.h"
#include "mud/settings.h"
#include "mud/object.h"
#include "mud/skill.h"
#include "mud/hooks.h"
#include "mud/efactory.h"

_MNpcBP MNpcBP;

NpcBP::NpcBP() : parent(NULL) {}

void NpcBP::reset_name()
{
	// clear
	name.set_name("an npc");
	set_flags.name = false;

	// get parent value
	const NpcBP* data = get_parent();
	if (data != NULL) {
		name = data->get_name();
	}
}

void NpcBP::reset_desc()
{
	// clear
	desc = std::string("npc");
	set_flags.desc = false;

	// get parent value
	const NpcBP* data = get_parent();
	if (data != NULL)
		desc = data->get_desc();
}

void NpcBP::reset_gender()
{
	// reset
	gender = GenderType::NONE;
	set_flags.gender = false;

	// get parent value
	const NpcBP* data = get_parent();
	if (data != NULL)
		gender = data->get_gender();
}

void NpcBP::reset_combat_dodge()
{
	// reset
	combat.dodge = 0;
	set_flags.dodge = false;

	// get parent
	const NpcBP* data = get_parent();
	if (data != NULL)
		combat.dodge = data->get_combat_dodge();
}

void NpcBP::reset_combat_attack()
{
	// reset
	combat.attack = 0;
	set_flags.attack = false;

	// get parent
	const NpcBP* data = get_parent();
	if (data != NULL)
		combat.attack = data->get_combat_attack();
}

void NpcBP::reset_combat_damage()
{
	// reset
	combat.damage = 0;
	set_flags.damage = false;

	// get parent
	const NpcBP* data = get_parent();
	if (data != NULL)
		combat.damage = data->get_combat_damage();
}

void NpcBP::reset_stats()
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

void NpcBP::refresh()
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

void NpcBP::set_parent(NpcBP* blueprint)
{
	parent = blueprint;
	refresh();
}

int NpcBP::load(File::Reader& reader)
{
	FO_READ_BEGIN
	FO_ATTR("blueprint", "id")
	id = node.get_string();
	FO_ATTR("blueprint", "parent")
	NpcBP* blueprint = MNpcBP.lookup(node.get_string());
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

void NpcBP::save(File::Writer& writer)
{
	if (set_flags.name)
		writer.attr("blueprint", "name", name.get_name());

	for (std::vector<std::string>::const_iterator i = keywords.begin(); i != keywords.end(); ++i)
		writer.attr("blueprint", "keyword", *i);

	if (set_flags.desc)
		writer.attr("blueprint", "desc", desc);

	if (set_flags.gender)
		writer.attr("blueprint", "gender", gender.get_name());

	if (set_flags.stats)
		for (int i = 0; i < CreatureStatID::COUNT; ++i)
			writer.attr("stat", CreatureStatID(i).get_name(), base_stats[i]);

	if (set_flags.dodge)
		writer.attr("combat", "dodge", combat.dodge);
	if (set_flags.attack)
		writer.attr("combat", "attack", combat.attack);
	if (set_flags.damage)
		writer.attr("combat", "damage", combat.damage);
}

int _MNpcBP::initialize()
{
	// requirements
	if (require(MObjectBP) != 0)
		return 1;
	if (require(MEvent) != 0)
		return 1;

	std::vector<std::string> files = File::dirlist(MSettings.get_blueprint_path());
	File::filter(files, "*.npcs");
	for (std::vector<std::string>::iterator i = files.begin(); i != files.end(); ++i) {
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

		if (blueprint->get_id().empty()) {
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

void _MNpcBP::shutdown()
{
	for (BlueprintMap::iterator i = blueprints.begin(), e = blueprints.end();
	        i != e; ++i)
		delete i->second;
}

NpcBP* _MNpcBP::lookup(const std::string& id)
{
	BlueprintMap::iterator iter = blueprints.find(id);
	if (iter == blueprints.end())
		return NULL;
	else
		return iter->second;
}
