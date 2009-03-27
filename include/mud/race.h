/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef RACE_H
#define RACE_H

#include "mud/fileobj.h"
#include "mud/creature.h"
#include "mud/form.h"
#include "mud/server.h"
#include "common/imanager.h"

/* store information about a race */
class
			Race
{
public:
	// this is an ugly hack, but it lets us construct the linked
	// list as we need to do
	Race(const std::string& s_name, Race* s_next);

	int load(File::Reader&);

	int getLifeSpan() const { return life_span; }
	int getAgeMin() const { return age_min; }
	int getAgeMax() const { return age_max; }

	int getStat(int i) const { if (i >= 0 && i < CreatureStatID::COUNT) return stats[i] ; else return 0; }

	int getAverageHeight(GenderType gender) const { return height[gender.getValue()]; }

	std::string getName() const { return name; }
	std::string getAdj() const { return adj; }
	std::string getBody() const { return body; }
	std::string getAbout() const { return about; }
	std::string getDesc() const { return desc; }

	const std::vector<FormColor>& getEyeColors() const { return eye_colors; }
	const std::vector<FormColor>& getSkinColors() const { return skin_colors; }
	const std::vector<FormColor>& getHairColors() const { return hair_colors; }

	Race* getNext() const { return next; }

	// ---- data ----
protected:
	std::string name;
	std::string adj;
	std::string body;
	std::string about;
	std::string desc;

	int age_min, age_max, life_span;

	int height[GenderType::COUNT];

	int stats[CreatureStatID::COUNT];

	std::vector<FormColor> eye_colors;
	std::vector<FormColor> hair_colors;
	std::vector<FormColor> skin_colors;

	Race* next;
};

class _MRace : public IManager
{
public:
	_MRace() : head(NULL) {}

	int initialize();
	void shutdown();

	Race* get(const std::string& name);
	Race* first() { return head; }

private:
	Race* head;
};
extern _MRace MRace;

#endif
