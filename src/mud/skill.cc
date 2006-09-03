/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <stdlib.h>

#include "mud/skill.h"
#include "mud/filetab.h"
#include "mud/settings.h"
#include "mud/skill.h"
#include "common/log.h"

#include <algorithm>

SSkillManager SkillManager;

namespace {
	class SkillSort {
		public:
		inline bool operator ()(const SkillInfo* left, const SkillInfo* right) {
			return left->get_name() < right->get_name();
		}
	};
}

SCRIPT_TYPE(Skill);
SkillInfo::SkillInfo () : Scriptix::Native(AweMUD_SkillType), id(0), type(SKILL_TYPE_NORMAL) { }

int
SSkillManager::initialize (void)
{
	File::TabReader reader;

	// Open skills file
	String path = SettingsManager.get_misc_path() + "/skills";
	if (reader.open(path)) {
		Log::Error << "Failed to open " << path;
		return 1;
	}
	if (reader.load()) {
		Log::Error << "Failed to parse" << path;
		return 1;
	}

	// process all skills
	for (size_t i = 0; i < reader.size(); ++i) {
		SkillInfo* skill = new SkillInfo();
		skill->short_name = reader.get(i, 0);
		String type = reader.get(i, 1);
		skill->name = reader.get(i, 2);
		skill->desc = reader.get(i, 3);

		if (skill->short_name.empty() || skill->name.empty() || skill->desc.empty()) {
			Log::Info << "Incomplete skill at " << path << ":" << reader.get_line(i);
			continue;
		}

		if (type == "normal")
			skill->type = SKILL_TYPE_NORMAL;
		else if (type == "intrinsic")
			skill->type = SKILL_TYPE_INTRINSIC;
		else if (type == "restricted")
			skill->type = SKILL_TYPE_RESTRICTED;
		else if (type == "locked")
			skill->type = SKILL_TYPE_LOCKED;
		else if (type == "secret")
			skill->type = SKILL_TYPE_SECRET;
		else {
			Log::Info << "Invalid skill type '" << type << "' at " << path << ":" << reader.get_line(i);
			continue;
		}

		// add skill
		skill_list.push_back(skill);
	}

	reader.close();

	// Sort skills by name
	std::sort(skill_list.begin(), skill_list.end(), SkillSort());

	// Assign skill IDs, add to name-lookup map
	SkillID id = 0;
	for (SkillList::iterator i = skill_list.begin(); i != skill_list.end(); ++i) {
		(*i)->id = ++id;
		skill_name_map[(*i)->get_name()] = *i;
		skill_short_name_map[(*i)->get_short_name()] = *i;
	}

	return 0;
}

void
SSkillManager::shutdown (void)
{
	skill_name_map.clear();
	skill_short_name_map.clear();
	skill_list.clear();
}

SkillInfo*
SSkillManager::get_by_name (String name)
{
	SkillMap::iterator i = skill_name_map.find(name);
	if (i != skill_name_map.end())
		return i->second;
	return NULL;
}

SkillInfo*
SSkillManager::get_by_short_name (String name)
{
	SkillMap::iterator i = skill_short_name_map.find(name);
	if (i != skill_short_name_map.end())
		return i->second;
	return NULL;
}

SkillInfo*
SSkillManager::get_by_id (SkillID id)
{
	if (id < 1 || id > skill_list.size())
		return NULL;

	return skill_list[id - 1];
}

SkillSet::SkillSet ()
{
	skills.insert(skills.begin(), SkillManager.get_size(), 0);
}

uint8
SkillSet::get_skill (SkillID id) const
{
	if (id < 1 || id > skills.size())
		return 0;

	return skills[id - 1];
}

uint8
SkillSet::set_skill (SkillID id, uint8 value)
{
	if (id < 1 || id > skills.size())
		return 0;

	return skills[id - 1] = value;
}
