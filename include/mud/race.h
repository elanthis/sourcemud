/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef RACE_H
#define RACE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>

#include "common/gcvector.h"
#include "common/gcmap.h"
#include "common/gcset.h"
#include "common/string.h"
#include "mud/fileobj.h"
#include "mud/char.h"
#include "mud/pdesc.h"
#include "mud/server.h"
#include "common/imanager.h"
#include "scriptix/native.h"

/* store information about a race */
class
Race : public Scriptix::Native
{
	public:
	// this is an ugly hack, but it lets us construct the linked
	// list as we need to do
	Race (StringArg s_name, Race *s_next);

	int load (File::Reader&);

	int get_rand_age (void) const;
	inline int get_life_span (void) const { return life_span; }
	inline int get_stat (int i) const { if (i >= 0 && i < CharStatID::COUNT) return stats[i] ; else return 0; }

	inline int get_average_height (GenderType gender) const { return height[gender.get_value()]; }

	inline const String& get_name (void) const { return name; }
	inline const String& get_adj (void) const { return adj; }
	inline const String& get_body (void) const { return body; }
	inline const String& get_about (void) const { return about; }
	inline const String& get_desc (void) const { return desc; }

	bool has_trait_value (CharacterTraitID trait, CharacterTraitValue value) const;
	const GCType::set<CharacterTraitValue>& get_trait_values (CharacterTraitID trait) const;
	
	inline const String& get_skin_type (void) const { return skin_type; }

	inline Race *get_next (void) const { return next; }

	// ---- data ----
	protected:
	String name;
	String adj;
	String body;
	String about;
	String desc;
	String skin_type;

	GCType::map<CharacterTraitID, GCType::set<CharacterTraitValue> > traits;

	int age_min, age_max, life_span;

	int height[GenderType::COUNT];

	int stats[CharStatID::COUNT];

	Race* next;
};

class SRaceManager : public IManager
{
	public:
	inline SRaceManager (void) : head(NULL) {}

	int initialize (void);

	void shutdown (void);

	Race *get (StringArg name);

	inline Race *first (void) { return head; }

	private:
	Race* head;
};
extern SRaceManager RaceManager;

#endif
