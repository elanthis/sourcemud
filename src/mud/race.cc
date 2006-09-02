/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common/rand.h"
#include "mud/race.h"
#include "mud/settings.h"
#include "mud/fileobj.h"
#include "common/log.h"
#include "mud/char.h"
#include "mud/pdesc.h"

SRaceManager RaceManager;

SCRIPT_TYPE(PlayerRace);
Race::Race (String s_name, Race *s_next) :
	Scriptix::Native(AweMUD_PlayerRaceType),
	name(s_name.c_str()),
	next(s_next) {}

int
Race::load (File::Reader& reader)
{
	// clear and/or defaults
	adj.clear();
	about = S("AweMUD player race.");
	desc.clear();
	age_min = 0;
	age_max = 0;
	life_span = 0;
	body = S("human");
	traits.clear();
	height[GenderType::NONE] = 72;
	height[GenderType::FEMALE] = 65;
	height[GenderType::MALE] = 68;
	for (int i = 0; i < CharStatID::COUNT; ++i)
		stats[i] = 0;

	FO_READ_BEGIN
		FO_ATTR2("race", "name")
			FO_TYPE_ASSERT(STRING)
			name = node.get_data();
		FO_ATTR2("race", "adj")
			FO_TYPE_ASSERT(STRING)
			adj = node.get_data();
		FO_ATTR2("race", "body")
			FO_TYPE_ASSERT(STRING)
			body = node.get_data();
		FO_ATTR2("race", "desc")
			FO_TYPE_ASSERT(STRING)
			desc = node.get_data();
		FO_ATTR2("race", "about")
			FO_TYPE_ASSERT(STRING)
			about = node.get_data();
		FO_ATTR2("race", "min_age")
			FO_TYPE_ASSERT(INT)
			age_min = tolong(node.get_data());
		FO_ATTR2("race", "max_age")
			FO_TYPE_ASSERT(INT)
			age_max = tolong(node.get_data());
		FO_ATTR2("race", "lifespan")
			FO_TYPE_ASSERT(INT)
			life_span = tolong(node.get_data());
		FO_ATTR2("race", "neuter_height")
			FO_TYPE_ASSERT(INT)
			height[GenderType::NONE] = tolong(node.get_data());
		FO_ATTR2("race", "female_height")
			FO_TYPE_ASSERT(INT)
			height[GenderType::FEMALE] = tolong(node.get_data());
		FO_ATTR2("race", "male_height")
			FO_TYPE_ASSERT(INT)
			height[GenderType::MALE] = tolong(node.get_data());
		FO_KEYED("trait")
			FO_TYPE_ASSERT(STRING)
			CharacterTraitID trait = CharacterTraitID::lookup(node.get_key());
			if (!trait.valid())
				throw File::Error(S("Invalid trait"));
			CharacterTraitValue value = CharacterTraitValue::lookup(node.get_data());
			if (!value.valid())
				throw File::Error(S("Invalid trait value"));

			GCType::map<CharacterTraitID, GCType::set<CharacterTraitValue> >::iterator i = traits.find(trait);
			if (i == traits.end()) {
				GCType::set<CharacterTraitValue> values;
				values.insert(value);
				traits[trait] = values;
				i = traits.find(trait);
			} else {
				std::pair<GCType::set<CharacterTraitValue>::iterator, bool> ret = i->second.insert(value);
			}
		FO_KEYED("stat")
			CharStatID stat = CharStatID::lookup(node.get_key());
			if (stat)
				stats[stat.get_value()] = tolong(node.get_data());
	FO_READ_ERROR
		return -1;
	FO_READ_END

	// fixup adjective
	if (!adj)
		adj = name;

	// sanity check age/lifespan
	if ((age_min > age_max) || (age_min <= 0) || (age_max >= life_span) || (life_span <= 0)) {
		Log::Error << "Minimum/maximum ages and/or lifespan in race '" << name << "' are invalid";
		return -1;
	}

	// ok
	return 0;
}

Race *
SRaceManager::get (String name)
{
	Race *race = head;
	while (race != NULL) {
		if (str_eq (race->get_name(), name))
			return race;
		race = race->get_next();
	}
	return NULL;
}

int
SRaceManager::initialize(void)
{
	require(CharacterTraitManager);

	File::Reader reader;
	String path = SettingsManager.get_misc_path() + "/races";

	if (reader.open(path)) {
		Log::Error << "Failed to open " << path;
		return 1;
	}

	// load
	FO_READ_BEGIN
		FO_OBJECT("race")
			// load race
			Race *race = new Race (node.get_name(), head);
			if (race->load (reader))
				return -1;
			head = race;
	FO_READ_ERROR
		return -1;
	FO_READ_END

	return 0;
}

void
SRaceManager::shutdown (void)
{
	head = NULL;
}
