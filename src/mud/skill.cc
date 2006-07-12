/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <stdlib.h>

#include "mud/skill.h"
#include "mud/fileobj.h"
#include "mud/settings.h"
#include "mud/skill.h"

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
SkillInfo::load (File::Reader& reader)
{
	FO_READ_BEGIN
		FO_ATTR("name")
			FO_TYPE_ASSERT(STRING)
			name = node.get_data();
		FO_ATTR("desc")
			FO_TYPE_ASSERT(STRING)
			desc = node.get_data();
		FO_ATTR("type")
			FO_TYPE_ASSERT(STRING)
			if (node.get_data() == "normal")
				type = SKILL_TYPE_NORMAL;
			else if (node.get_data() == "intrinsic")
				type = SKILL_TYPE_INTRINSIC;
			else if (node.get_data() == "restricted")
				type = SKILL_TYPE_RESTRICTED;
			else if (node.get_data() == "locked")
				type = SKILL_TYPE_LOCKED;
			else if (node.get_data() == "secret")
				type = SKILL_TYPE_SECRET;
			else
				throw File::Error(S("Invalid skill type"));
	FO_READ_ERROR
		return -1;
	FO_READ_END

	return 0;
}

int
SSkillManager::initialize (void)
{
	File::Reader reader;

	// Open skills file
	String path = SettingsManager.get_misc_path() + "/skills";
	if (reader.open(path)) {
		Log::Error << "Failed to open " << path;
		return 1;
	}

	// Load skills file
	FO_READ_BEGIN
		FO_OBJECT("skill")
			SkillInfo* skill = new SkillInfo();

			if (skill->load(reader))
				return -1;

			// Warn on duplicates
			if (get_by_name(skill->get_name()))
				Log::Warning << "Duplicate skill with name '" << skill->get_name() << "' in " << reader.get_filename() << ':' << node.get_line();

			// Add skill
			skill_list.push_back(skill);
	FO_READ_ERROR
		return 2;
	FO_READ_END

	reader.close();

	// Sort skills by name
	std::sort(skill_list.begin(), skill_list.end(), SkillSort());

	// Assign skill IDs, add to name-lookup map
	SkillID id = 0;
	for (SkillList::iterator i = skill_list.begin(); i != skill_list.end(); ++i) {
		(*i)->id = ++id;
		skill_map[(*i)->get_name()] = *i;
	}

	return 0;
}

void
SSkillManager::shutdown (void)
{
	skill_map.clear();
	skill_list.clear();
}

SkillInfo*
SSkillManager::get_by_name (String name)
{
	SkillMap::iterator i = skill_map.find(name);
	if (i != skill_map.end())
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
