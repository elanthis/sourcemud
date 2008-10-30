/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef MUD_SKILL_H 
#define MUD_SKILL_H

#include "common/types.h"
#include "common/string.h"
#include "generated/skills.h"

class SkillSet
{
	public:
	SkillSet ();

	uint8 get_skill (SkillID id) const;
	uint8 set_skill (SkillID id, uint8 value);
	bool has_skill (SkillID id) const { return get_skill(id) > 0; }

	private:
	uint8 skills[SkillID::COUNT];
};

#endif
