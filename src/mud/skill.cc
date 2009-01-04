/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "mud/skill.h"

std::vector<std::string> SkillID::names;

const SkillID SKILL_DODGE = SkillID::create("dodge");
const SkillID SKILL_PERCEPTION = SkillID::create("perception");

SkillSet::SkillSet()
{
}

uint8 SkillSet::getSkill(SkillID id) const
{
	if (!id)
		return 0;
	if (id >= (int)skills.size())
		return 0;

	return skills[id.getValue() - 1];
}

uint8 SkillSet::setSkill(SkillID id, uint8 value)
{
	if (!id)
		return 0;
	skills.reserve(id.getValue() - 1);

	return skills[id.getValue() - 1] = value;
}

SkillID SkillID::lookup(const std::string& name)
{
	for (size_t i = 1, e = names.size(); i < e; ++i)
		if (names[i] == name)
			return SkillID(i);
	return SkillID();
}

SkillID SkillID::create(const std::string& name)
{
	SkillID id = lookup(name);
	if (id)
		return id;

	names.push_back(name);
	return SkillID(names.size());
}
