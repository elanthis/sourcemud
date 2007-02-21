/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/skill.h"

SkillSet::SkillSet ()
{
	memset(skills, 0, sizeof(skills));
}

uint8
SkillSet::get_skill (SkillID id) const
{
	if (!id.valid())
		return 0;

	return skills[id.get_value() - 1];
}

uint8
SkillSet::set_skill (SkillID id, uint8 value)
{
	if (!id.valid())
		return 0;

	return skills[id.get_value() - 1] = value;
}
