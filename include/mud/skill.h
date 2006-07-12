/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef SKILL_H 
#define SKILL_H

#include "common/types.h"
#include "common/string.h"
#include "mud/server.h"
#include "common/imanager.h"
#include "common/gcbase.h"
#include "common/gcmap.h"
#include "common/gcvector.h"
#include "mud/fileobj.h"
#include "scriptix/native.h"

typedef unsigned int SkillID;

enum SkillType {
	SKILL_TYPE_NORMAL = 0,		// none
	SKILL_TYPE_RESTRICTED = 1,	// bits 1
	SKILL_TYPE_LOCKED = 3,		// bits 1 and 2
	SKILL_TYPE_INTRINSIC = 5,	// bits 1 and 3
	SKILL_TYPE_SECRET = 11,		// bits 1, 2, and 4
};

class SkillInfo : public Scriptix::Native
{
	public:
	SkillInfo ();

	inline SkillID get_id () const { return id; }
	inline const String& get_name () const { return name; }
	inline const String& get_desc () const { return desc; }

	inline bool is_intrinsic () const { return type & SKILL_TYPE_INTRINSIC; }
	inline bool is_restricted () const { return type & SKILL_TYPE_RESTRICTED; }
	inline bool is_secret () const { return type & SKILL_TYPE_SECRET; }
	inline bool is_locked () const { return type & SKILL_TYPE_LOCKED; }

	int load (File::Reader& reader);

	private:
	SkillID id;
	String name;
	String desc;
	SkillType type;

	friend class SSkillManager;
};

class SkillSet
{
	public:
	SkillSet ();

	uint8 get_skill (SkillID id) const;
	uint8 set_skill (SkillID id, uint8 value);
	inline bool has_skill (SkillID id) const { return get_skill(id) > 0; }

	private:
	GCType::vector<uint8> skills;
};

class SSkillManager : public IManager
{
	public:
	typedef GCType::map<String, SkillInfo*> SkillMap;
	typedef GCType::vector<SkillInfo*> SkillList;

	int initialize ();

	void shutdown ();

	SkillInfo* get_by_name (String name);
	SkillInfo* get_by_id (SkillID id);

	inline size_t get_size () { return skill_list.size(); }

	const SkillList& get_skills () const { return skill_list; }

	private:
	SkillMap skill_map;
	SkillList skill_list;
};

extern SSkillManager SkillManager;

#endif
