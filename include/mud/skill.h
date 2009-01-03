/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef MUD_SKILL_H
#define MUD_SKILL_H

#include "common/types.h"

class SkillID
{
public:
	explicit SkillID(int s_value) : value(s_value) {}
	SkillID(const SkillID& event) : value(event.value) {}
	SkillID() : value(0) {}

	std::string getName() const { return names[value]; }
	int getValue() const { return value; }
	static size_t size() { return names.size(); }

	static SkillID lookup(const std::string& name);
	static SkillID create(const std::string& name);

	operator bool() const { return value; }
	bool operator ==(SkillID dir) const { return dir.value == value; }
	bool operator !=(SkillID dir) const { return dir.value != value; }
	bool operator <(SkillID dir) const { return value < dir.value; }

private:
	int value;

	static std::vector<std::string> names;
};

class SkillSet
{
public:
	SkillSet();

	uint8 getSkill(SkillID id) const;
	uint8 setSkill(SkillID id, uint8 value);
	bool hasSkill(SkillID id) const { return getSkill(id) > 0; }

private:
	std::vector<int> skills;
};

extern const SkillID SKILL_DODGE;
extern const SkillID SKILL_PERCEPTION;

#endif
