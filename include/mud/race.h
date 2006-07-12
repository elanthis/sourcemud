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

typedef GCType::map<CharacterTraitID, GCType::set<CharacterTraitValue> > RaceTraitMap;

/* store information about a race */
class
Race : public Scriptix::Native
{
	public:
	// this is an ugly hack, but it lets us construct the linked
	// list as we need to do
	Race (String s_name, Race* s_next);

	int load (File::Reader&);

	inline int get_life_span () const { return life_span; }
	inline int get_age_min () const { return age_min; }
	inline int get_age_max () const { return age_max; }

	inline int get_stat (int i) const { if (i >= 0 && i < CharStatID::COUNT) return stats[i] ; else return 0; }

	inline int get_average_height (GenderType gender) const { return height[gender.get_value()]; }

	inline const String& get_name () const { return name; }
	inline const String& get_adj () const { return adj; }
	inline const String& get_body () const { return body; }
	inline const String& get_about () const { return about; }
	inline const String& get_desc () const { return desc; }

	const RaceTraitMap& get_traits () const { return traits; }
	
	inline Race* get_next () const { return next; }

	// ---- data ----
	protected:
	String name;
	String adj;
	String body;
	String about;
	String desc;

	RaceTraitMap traits;

	int age_min, age_max, life_span;

	int height[GenderType::COUNT];

	int stats[CharStatID::COUNT];

	Race* next;
};

class SRaceManager : public IManager
{
	public:
	inline SRaceManager () : head(NULL) {}

	int initialize ();

	void shutdown ();

	Race* get (String name);

	inline Race* first () { return head; }

	private:
	Race* head;
};
extern SRaceManager RaceManager;

#endif
