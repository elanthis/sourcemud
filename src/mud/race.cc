/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/rand.h"
#include "common/log.h"
#include "common/string.h"
#include "mud/race.h"
#include "mud/settings.h"
#include "mud/fileobj.h"
#include "mud/creature.h"
#include "mud/form.h"

_MRace MRace;

Race::Race(const std::string& s_name, Race *s_next) :
		name(s_name.c_str()),
		next(s_next) {}

int Race::load(File::Reader& reader)
{
	// clear and/or defaults
	adj.clear();
	about = "Source MUD player race.";
	desc.clear();
	age_min = 0;
	age_max = 0;
	life_span = 0;
	body = "human";
	height[GenderType::NONE] = 72;
	height[GenderType::FEMALE] = 65;
	height[GenderType::MALE] = 68;
	for (int i = 0; i < CreatureStatID::COUNT; ++i)
		stats[i] = 0;

	FO_READ_BEGIN
	FO_ATTR("race", "name")
	name = node.get_string();
	FO_ATTR("race", "adj")
	adj = node.get_string();
	FO_ATTR("race", "body")
	body = node.get_string();
	FO_ATTR("race", "desc")
	desc = node.get_string();
	FO_ATTR("race", "about")
	about = node.get_string();
	FO_ATTR("race", "eye_color")
	eye_colors.push_back(FormColor::create(node.get_string()));
	FO_ATTR("race", "hair_color")
	hair_colors.push_back(FormColor::create(node.get_string()));
	FO_ATTR("race", "skin_color")
	skin_colors.push_back(FormColor::create(node.get_string()));
	FO_ATTR("race", "min_age")
	age_min = node.get_int();
	FO_ATTR("race", "max_age")
	age_max = node.get_int();
	FO_ATTR("race", "lifespan")
	life_span = node.get_int();
	FO_ATTR("race", "neuter_height")
	height[GenderType::NONE] = node.get_int();
	FO_ATTR("race", "female_height")
	height[GenderType::FEMALE] = node.get_int();
	FO_ATTR("race", "male_height")
	height[GenderType::MALE] = node.get_int();
	FO_ATTR("race", "stat_bonus")
	CreatureStatID stat = CreatureStatID::lookup(node.get_string(0));
	if (stat)
		stats[stat.get_value()] = node.get_int(1);
	FO_READ_ERROR
	return -1;
	FO_READ_END

	// fixup adjective
	if (adj.empty())
		adj = name;

	// sanity check age/lifespan
	if ((age_min > age_max) || (age_min <= 0) || (age_max >= life_span) || (life_span <= 0)) {
		Log::Error << "Minimum/maximum ages and/or lifespan in race '" << name << "' are invalid";
		return -1;
	}

	// ok
	return 0;
}

Race* _MRace::get(const std::string& name)
{
	Race *race = head;
	while (race != NULL) {
		if (str_eq(race->get_name(), name))
			return race;
		race = race->get_next();
	}
	return NULL;
}

int _MRace::initialize()
{
	File::Reader reader;
	std::string path = MSettings.get_misc_path() + "/races";

	if (reader.open(path)) {
		Log::Error << "Failed to open " << path;
		return 1;
	}

	// load
	FO_READ_BEGIN
	FO_OBJECT("races", "race")
	// load race
	Race *race = new Race(node.get_name(), head);
	if (race->load(reader))
		return -1;
	head = race;
	FO_READ_ERROR
	return -1;
	FO_READ_END

	return 0;
}

void _MRace::shutdown()
{
	while (head != NULL) {
		Race* r = head->get_next();
		delete head;
		head = r;
	}
}
