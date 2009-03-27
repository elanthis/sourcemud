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

void NpcBP::resetName()
{
	// clear
	name.setFull("an npc");
	set_flags.name = false;

	// get parent value
	const NpcBP* data = getParent();
	if (data != NULL) {
		name = data->getName();
	}
}

void NpcBP::resetDesc()
{
	// clear
	desc = std::string("npc");
	set_flags.desc = false;

	// get parent value
	const NpcBP* data = getParent();
	if (data != NULL)
		desc = data->getDesc();
}

void NpcBP::resetGender()
{
	// reset
	gender = GenderType::NONE;
	set_flags.gender = false;

	// get parent value
	const NpcBP* data = getParent();
	if (data != NULL)
		gender = data->getGender();
}

void NpcBP::resetCombatDodge()
{
	// reset
	combat.dodge = 0;
	set_flags.dodge = false;

	// get parent
	const NpcBP* data = getParent();
	if (data != NULL)
		combat.dodge = data->getCombatDodge();
}

void NpcBP::resetCombatAttack()
{
	// reset
	combat.attack = 0;
	set_flags.attack = false;

	// get parent
	const NpcBP* data = getParent();
	if (data != NULL)
		combat.attack = data->getCombatAttack();
}

void NpcBP::resetCombatDamage()
{
	// reset
	combat.damage = 0;
	set_flags.damage = false;

	// get parent
	const NpcBP* data = getParent();
	if (data != NULL)
		combat.damage = data->getCombatDamage();
}

void NpcBP::resetStats()
{
	// reset
	for (int i = 0; i < CreatureStatID::COUNT; ++i)
		base_stats[i] = 0;
	set_flags.stats = false;

	// get parent
	const NpcBP* data = getParent();
	if (data != NULL)
		for (int i = 0; i < CreatureStatID::COUNT; ++i)
			base_stats[i] = data->getStat(i);
}

void NpcBP::refresh()
{
	if (!set_flags.name)
		resetName();
	if (!set_flags.desc)
		resetDesc();
	if (!set_flags.gender)
		resetGender();
	if (!set_flags.dodge)
		resetCombatDodge();
	if (!set_flags.attack)
		resetCombatAttack();
	if (!set_flags.damage)
		resetCombatDamage();
	if (!set_flags.stats)
		resetStats();
}

void NpcBP::setParent(NpcBP* blueprint)
{
	parent = blueprint;
	refresh();
}

int NpcBP::load(File::Reader& reader)
{
	FO_READ_BEGIN
	FO_ATTR("blueprint", "id")
	id = node.getString();
	FO_ATTR("blueprint", "parent")
	NpcBP* blueprint = MNpcBP.lookup(node.getString());
	if (blueprint)
		setParent(blueprint);
	else
		Log::Warning << "Undefined parent npc blueprint '" << node.getString() << "' at " << reader.getFilename() << ':' << node.getLine();
	FO_ATTR("blueprint", "equip")
	equip_list.push_back(node.getString());
	FO_ATTR("blueprint", "name")
	setName(node.getString());
	FO_ATTR("blueprint", "keyword")
	keywords.push_back(node.getString());
	FO_ATTR("blueprint", "desc")
	setDesc(node.getString());
	FO_ATTR("blueprint", "gender")
	setGender(GenderType::lookup(node.getString()));
	FO_ATTR("combat", "dodge")
	combat.dodge = node.getInt();
	set_flags.dodge = true;
	FO_ATTR("combat", "attack")
	combat.attack = node.getInt();
	set_flags.attack = true;
	FO_ATTR("combat", "damage")
	combat.damage = node.getInt();
	set_flags.damage = true;
	FO_ATTR("blueprint", "stat")
	CreatureStatID stat = CreatureStatID::lookup(node.getString(0));
	if (stat) {
		base_stats[stat.getValue()] = node.getInt(1);
	}
	FO_READ_ERROR
	return -1;
	FO_READ_END

	return 0;
}

void NpcBP::save(File::Writer& writer)
{
	if (set_flags.name)
		writer.attr("blueprint", "name", name.getFull());

	for (std::vector<std::string>::const_iterator i = keywords.begin(); i != keywords.end(); ++i)
		writer.attr("blueprint", "keyword", *i);

	if (set_flags.desc)
		writer.attr("blueprint", "desc", desc);

	if (set_flags.gender)
		writer.attr("blueprint", "gender", gender.getName());

	if (set_flags.stats)
		for (int i = 0; i < CreatureStatID::COUNT; ++i)
			writer.attr("stat", CreatureStatID(i).getName(), base_stats[i]);

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

	std::vector<std::string> files = File::dirlist(MSettings.getBlueprintPath());
	File::filter(files, "*.npcs");
	for (std::vector<std::string>::iterator i = files.begin(); i != files.end(); ++i) {
		File::Reader reader;
		if (reader.open(*i))
			return -1;
		FO_READ_BEGIN
		FO_OBJECT("blueprint", "npc")
		NpcBP* blueprint = new NpcBP();
		if (blueprint->load(reader)) {
			Log::Warning << "Failed to load blueprint in " << reader.getFilename() << " at " << node.getLine();
			return -1;
		}

		if (blueprint->getId().empty()) {
			Log::Warning << "Blueprint has no ID in " << reader.getFilename() << " at " << node.getLine();
			return -1;
		}

		blueprints[blueprint->getId()] = blueprint;
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
