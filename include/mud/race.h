/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
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
#include "mud/creature.h"
#include "mud/form.h"
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
	Race (String s_name, Race* s_next);

	int load (File::Reader&);

	int get_life_span () const { return life_span; }
	int get_age_min () const { return age_min; }
	int get_age_max () const { return age_max; }

	int get_stat (int i) const { if (i >= 0 && i < CreatureStatID::COUNT) return stats[i] ; else return 0; }

	int get_average_height (GenderType gender) const { return height[gender.get_value()]; }

	String get_name () const { return name; }
	String get_adj () const { return adj; }
	String get_body () const { return body; }
	String get_about () const { return about; }
	String get_desc () const { return desc; }

	const GCType::vector<FormColor>& get_eye_colors() const { return eye_colors; }
	const GCType::vector<FormColor>& get_skin_colors() const { return skin_colors; }
	const GCType::vector<FormColor>& get_hair_colors() const { return hair_colors; }

	Race* get_next () const { return next; }

	// ---- data ----
	protected:
	String name;
	String adj;
	String body;
	String about;
	String desc;

	int age_min, age_max, life_span;

	int height[GenderType::COUNT];

	int stats[CreatureStatID::COUNT];

	GCType::vector<FormColor> eye_colors;
	GCType::vector<FormColor> hair_colors;
	GCType::vector<FormColor> skin_colors;

	Race* next;
};

class SRaceManager : public IManager
{
	public:
	SRaceManager () : head(NULL) {}

	int initialize ();

	void shutdown ();

	Race* get (String name);

	Race* first () { return head; }

	private:
	Race* head;
};
extern SRaceManager RaceManager;

#endif
