/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/telnet.h"
#include "mud/player.h"
#include "mud/login.h"
#include "mud/creation.h"
#include "mud/race.h"
#include "common/rand.h"

// STAT ROLLING CONFIGURATION
const int STAT_TOKENS = CharStatID::COUNT; // how many 'tokens' the player can spend on stats
const int STAT_POINTS = STAT_TOKENS * 25; // how many stat points the player gets randomly assigned
const int STAT_RAND_INC = 5; // the increment by which stats are advanced during random phase
const int STAT_TOKEN_INC = 5; // the increment by which stats are advanced during token phase
const int STAT_BASE = 30; // base value of all stats, before point distribution
const int STAT_MAX = 90; // maximum stat can be raised to in character creation

bool TelnetModeNewCharacter::is_match (String test, String operand)
{
	return !operand.empty() && !strncasecmp(test.c_str(), operand.c_str(), operand.size());
}

int TelnetModeNewCharacter::initialize ()
{
	// clear all variables
	name.clear();

	// begin
	enter_state(STATE_BEGIN);
	return 0;
}

void TelnetModeNewCharacter::shutdown ()
{
}

// DISPLAY CURRENT STATE'S PROMPT
void TelnetModeNewCharacter::prompt ()
{
	switch (state) {
		case STATE_NAME:
		case STATE_RENAME:
			*get_handler() << "Enter thy name:";
			break;
		case STATE_RACE:
			*get_handler() << "Choose thy race:";
			break;
		case STATE_GENDER:
			*get_handler() << "Choose thy gender:";
			break;
		case STATE_HEIGHT:
			*get_handler() << "Choose thy stature:";
			break;
		case STATE_TRAITS:
		{
			const RaceTraitMap& all = race->get_traits();
			for (RaceTraitMap::const_iterator i = all.begin(); i != all.end(); ++i) {
				if (traits.find(i->first) == traits.end()) {
					*get_handler() << "Choose thy " << i->first.name() << ':';
					break;
				}
			}
	  		break;
		}
		case STATE_STATS:
			*get_handler() << "Which attribute shall thee increase?";
			break;
		case STATE_NAME_CONFIRM:
		case STATE_RACE_CONFIRM:
		case STATE_TRAITS_CONFIRM:
		case STATE_STATS_CONFIRM:
		case STATE_FINAL_CONFIRM:
		case STATE_RENAME_CONFIRM:
			*get_handler() << "Accept? (Y/n)";
			break;
		case STATE_CONTINUE:
			*get_handler() << "Thy profile is complete. (Press ENTER to continue)";
			break;
	}
}

// PROCESS INPUT FOR CURRENT STATE
void TelnetModeNewCharacter::process (char* line)
{
	String input = strlower(S(line));
	int numeric = tolong(input);

	if (input == S("quit")) {
		finish();
		return;
	}

	switch (state) {
		case STATE_NAME:
		case STATE_RENAME:
			// must be a valid name
			if (!PlayerManager.valid_name(input)) {
				show_error(S("Thy chosen name is not acceptable."));
				break;
			}

			// not already in use
			if (PlayerManager.exists(input)) {
				show_error(S("Thy chosen name is already in use."));
				break;
			}

			// go to confirmation
			name = capwords(input);
			enter_state(state == STATE_NAME ? STATE_NAME_CONFIRM : STATE_RENAME_CONFIRM);
			break;
		case STATE_NAME_CONFIRM:
			if (!input || is_match(S("yes"), input))
				enter_state(state == STATE_NAME_CONFIRM ? STATE_RACE : STATE_FINAL_CONFIRM);
			else if (is_match(S("no"), input))
				enter_state(STATE_NAME);
			else
				show_error(S("I do not understand thy response."));
			break;
		case STATE_RACE:
		{
			// find the selected race
			Race* rptr = RaceManager.first();
			int index = 1;
			int selected = numeric;
			while (rptr != NULL) {
				if (phrase_match(rptr->get_name(), input) || index == selected)
					break;
				rptr = rptr->get_next();
				++ index;
			}
			if (rptr == NULL) {
				show_error(S("I do not understand thy response."));
				break;
			}

			// set race, confirm
			race = rptr;
			enter_state(STATE_RACE_CONFIRM);
			break;
		}
		case STATE_RACE_CONFIRM:
			if (!input || is_match(S("yes"), input))
				enter_state(STATE_GENDER);
			else if (is_match(S("no"), input))
				enter_state(STATE_RACE);
			else
				show_error(S("I do not understand thy response."));
			break;
		case STATE_GENDER:
			if (numeric == 1 || is_match(S("female"), input)) {
				gender = GenderType::FEMALE;
				enter_state(STATE_HEIGHT);
			} else if (numeric == 2 || is_match(S("male"), input)) {
				gender = GenderType::MALE;
				enter_state(STATE_HEIGHT);
			} else {
				show_error(S("I do not understand thy response."));
			}
			break;
		case STATE_HEIGHT:
		{
			// determine input
			if (numeric == 2 || is_match(S("short"), input)) {
				height = HEIGHT_SHORT;
			} else if (numeric == 1 || is_match(S("very short"), input)) {
				height = HEIGHT_VERY_SHORT;
			} else if (numeric == 4 || is_match(S("tall"), input)) {
				height = HEIGHT_TALL;
			} else if (numeric == 5 || is_match(S("very tall"), input)) {
				height = HEIGHT_VERY_TALL;
			} else if (numeric == 3 || is_match(S("average"), input)) {
				height = HEIGHT_AVERAGE;
			} else {
				show_error(S("I do not understand they response."));
				break;
			}

			// go to last step
			enter_state(STATE_TRAITS);
			break;
		}
		case STATE_TRAITS:
		{
			// determine current trait
			const RaceTraitMap& all = race->get_traits();
			RaceTraitMap::const_iterator trait;
			for (trait = all.begin(); trait != all.end(); ++trait) {
				if (traits.find(trait->first) == traits.end())
					break;
			}
			assert(trait != all.end());

			// set trait
			int index = 1;
			for (GCType::set<CharacterTraitValue>::const_iterator i = trait->second.begin(); i != trait->second.end(); ++i, ++index) {
				if (numeric == index || is_match(i->get_name(), input)) {
					traits.insert(std::pair<CharacterTraitID,CharacterTraitValue>(trait->first, *i));
					++trait; // now we need the next trait
					break;
				}
			}

			// if we found/set the trait, and are now out of traits, go to confirmation
			if (trait == all.end())
				enter_state(STATE_TRAITS_CONFIRM);
			else
				display();
	  		break;
		}
		case STATE_TRAITS_CONFIRM:
			if (!input || is_match(S("yes"), input))
				enter_state(STATE_STATS);
			else if (is_match(S("no"), input))
				enter_state(STATE_GENDER); // redo gender and height, too
			else
				show_error(S("I do not understand thy response."));
			break;
		case STATE_STATS:
		{
			// reroll?
			if (input == S("reroll") || input == S("reset")) {
				enter_state(STATE_STATS);
				break;
			}

			// determine chosen stat
			int stat;
			if (numeric >= 1 && numeric <= CharStatID::COUNT) {
				stat = numeric - 1;
			} else {
				for (stat = 0; stat < CharStatID::COUNT; ++stat) {
					if (is_match(CharStatID(stat).get_name(), input))
						break;
				}
				if (stat == CharStatID::COUNT) {
					show_error(S("I do not understand thy response."));
					break;
				}
			}

			// make sure stat is under limit
			if (stats[stat] >= STAT_MAX) {
				show_error(S("Ye may not increase that attribute any further."));
				break;
			}

			// increase stat
			stats[stat] += STAT_TOKEN_INC;

			// remove token, confirm if we're out
			--tokens;
			if (tokens == 0)
				enter_state(STATE_STATS_CONFIRM);
			else
				display();
			break;
		}
		case STATE_STATS_CONFIRM:
			if (!input || is_match(S("yes"), input))
				enter_state(STATE_FINAL_CONFIRM);
			else if (is_match(S("no"), input))
				enter_state(STATE_STATS);
			else
				show_error(S("I do not understand thy response."));
			break;
		case STATE_FINAL_CONFIRM:
			if (!input || is_match(S("yes"), input)) {
				if (PlayerManager.exists(name))
					enter_state(STATE_RENAME);
				else
					create();
			} else if (is_match(S("no"), input)) {
				enter_state(STATE_BEGIN);
			} else {
				show_error(S("I do not understand thy response."));
			}
			break;
		default:
			finish();
			break;
	}
}

void TelnetModeNewCharacter::display ()
{
	get_handler()->clear_scr();
	*get_handler() << 	S("Character Creation\n------------------\n");
	if (name)
		*get_handler() << "Name: " << name << "\n";
	if (race)
		*get_handler() << "Race: " << capwords(race->get_name()) << "\n";
	if (state > STATE_GENDER)
		*get_handler() << "Gender: " << capwords(gender.get_name()) << "\n";
	*get_handler() << "\n";

	switch (state) {
		case STATE_NAME:
			*get_handler() << "Choose the name ye would like thy new charater to be called.\n\n";
			break;
		case STATE_RENAME:
			*get_handler() << "Thy chosen name has been taken by another.  Choose ye a new name.\n\n";
			break;
		case STATE_RACE:
		{
			Race* rptr = RaceManager.first();
			int index = 1;
			while (rptr != NULL) {
				*get_handler() << index << ") " << capwords(rptr->get_name()) << "\n";
				rptr = rptr->get_next();
				++ index;
			}
			*get_handler() << "\n";
			break;
		}
		case STATE_RACE_CONFIRM:
		{
			get_handler()->set_indent(2);
			for (int i = 0; i < CharStatID::COUNT; ++i) {
				if (race->get_stat(i) != 0) {
					*get_handler() << CharStatID(i).get_name() << ':';
					get_handler()->set_indent(16);
					if (race->get_stat(i) > 0)
						*get_handler() << '+';
					else if (race->get_stat(i) < 0)
						*get_handler() << '-';
					else
						*get_handler() << ' ';
					if (abs(race->get_stat(i)) < 10)
						*get_handler() << ' ';
					*get_handler() << abs(race->get_stat(i)) << "\n";
					get_handler()->set_indent(2);
				}
			}
			get_handler()->set_indent(0);

			*get_handler() << "\nLife span: ";
			*get_handler() << race->get_life_span() << " years\n";

			// About
			*get_handler() << "\n" << CDESC << race->get_about() << CNORMAL << "\n\n";
			break;
		}
		case STATE_GENDER:
			*get_handler() << "1) Female\n2) Male\n\n";
			break;
		case STATE_TRAITS:
		{
			const RaceTraitMap& all = race->get_traits();
			for (RaceTraitMap::const_iterator i = all.begin(); i != all.end(); ++i) {
				if (traits.find(i->first) == traits.end()) {
					int index = 1;
					*get_handler() << capwords(i->first.name()) << ":\n";
					for (GCType::set<CharacterTraitValue>::const_iterator ii = i->second.begin(); ii != i->second.end(); ++ii, ++index) {
						*get_handler() << index << ") " << capwords(ii->get_name()) << "\n";
					}
					*get_handler() << "\n";
					break;
				}
			}
	  		break;
		}
		case STATE_TRAITS_CONFIRM:
			for (TraitMap::const_iterator i = traits.begin(); i != traits.end(); ++i) {
				*get_handler() << capwords(i->first.name()) << ": " << capwords(i->second.get_name()) << "\n";
			}
			break;
		case STATE_STATS:
		case STATE_STATS_CONFIRM:
		{
			*get_handler() << 
				"You have " << STAT_TOKENS << " tokens with which to increase your "
				"character attributes.  Each token increases an attribute "
				"by " << STAT_TOKEN_INC << " points.  Your base attributes are "
				"determined randomly; all characters will have the same total "
				"of attribute points, however.  You may at any time reroll "
				"your stats by typing 'reroll'.\n\n";

			for (int i = 0; i < CharStatID::COUNT; ++i) {
				int mod = (stats[i] - 50) / 10;

				// name
				*get_handler() << "  " << (i + 1) << ") " << CharStatID(i).get_name() << ": ";

				// descriptor
				get_handler()->set_indent(14);
				*get_handler() << get_stat_color(stats[i]) << get_stat_level(stats[i]) << CNORMAL;

				// value/+mod
				get_handler()->set_indent(25);
				*get_handler() << '(' << stats[i] << '/';
				if (mod >= 0)
					*get_handler() << '+';
				*get_handler() << mod << ")\n";

				get_handler()->set_indent(0);
			}

			// display remaining tokens
			if (tokens > 0)
				*get_handler() << "\nYe have " << tokens << " tokens remaining.\n\n";
			else
				*get_handler() << "\nThy tokens have been spent.\n\n";
			break;
		}
		case STATE_HEIGHT:
			*get_handler() <<
				"1) Very short\n"
				"2) Short\n"
				"3) Average\n"
				"4) Tall\n"
				"5) Very Tall\n\n";
			break;
		case STATE_FINAL_CONFIRM:
		case STATE_RENAME_CONFIRM:
			// display all details
			if (state > STATE_HEIGHT) {
				switch (height) {
					case HEIGHT_VERY_SHORT:
						*get_handler() << "Stature: Very short\n";
						break;
					case HEIGHT_SHORT:
						*get_handler() << "Stature: Short\n";
						break;
					case HEIGHT_AVERAGE:
						*get_handler() << "Stature: Average\n";
						break;
					case HEIGHT_TALL:
						*get_handler() << "Stature: Tall\n";
						break;
					case HEIGHT_VERY_TALL:
						*get_handler() << "Stature: Very tall\n";
						break;
				}
			}
			for (TraitMap::const_iterator i = traits.begin(); i != traits.end(); ++i) {
				*get_handler() << capwords(i->first.name()) << ": " << capwords(i->second.get_name()) << "\n";
			}
			if (state > STATE_STATS_CONFIRM) {
				*get_handler() << "Attributes: ";
				for (int i = 0; i < CharStatID::COUNT; ++i) {
					if (i > 0)
						*get_handler() << ", ";
					*get_handler() << CharStatID(i).get_name() << '(' << stats[i] << ')';
				}
				*get_handler() << '\n';
			}

			*get_handler() << "Is it thy wish for me to create this profile?\n\n";
			break;
		case STATE_CONTINUE:
			*get_handler() << " " CADMIN "** Character Created **" CNORMAL "\n\n";
			break;
		default:
			break;
	}
}

void TelnetModeNewCharacter::show_error (String msg)
{
	display();
	*get_handler() << CWARNING << msg << CNORMAL << "\n\n";
}

void TelnetModeNewCharacter::enter_state (state_t new_state)
{
	state = new_state;

	switch (state) {
		case STATE_NAME:
			name.clear();
			break;
		case STATE_RACE:
			race = NULL;
			break;
		case STATE_TRAITS:
		{
			// clear all traits
			traits.clear();

			// set any traits with only one option
			const RaceTraitMap& all = race->get_traits();
			for (RaceTraitMap::const_iterator i = all.begin(); i != all.end(); ++i) {
				if (i->second.size() == 1) {
					traits.insert(std::pair<CharacterTraitID,CharacterTraitValue>(i->first, *i->second.begin()));
				}
			}
			break;
		}
		case STATE_STATS:
		{
			tokens = STAT_TOKENS;
			for (int i = 0; i < CharStatID::COUNT; ++i)
				stats[i] = STAT_BASE;
			for (int i = STAT_POINTS; i > 0; i -= STAT_RAND_INC)
				stats[get_random(CharStatID::COUNT)] += STAT_RAND_INC;
			break;
		}
		default:
			break;
	}

	display();
}

void TelnetModeNewCharacter::create ()
{
	Player* player;

	// create the player
	player = new Player(account, name);

	// set basics
	player->set_race(race);
	player->set_gender(gender);

	// set birthday
	int age = race->get_age_min() + get_random(race->get_age_max() - race->get_age_min());
	GameTime birthday = TimeManager.time;
	birthday.set_year(birthday.get_year() - age);
	player->set_birthday(birthday);

	// set height
	int adjust;
	switch (height) {
		case HEIGHT_VERY_SHORT: adjust = -12; break;
		case HEIGHT_SHORT: adjust = -6; break;
		case HEIGHT_AVERAGE: adjust = 0; break;
		case HEIGHT_TALL: adjust = -6; break;
		case HEIGHT_VERY_TALL: adjust = -12; break;
	}
	player->set_height (race->get_average_height(gender) - 3 + get_random(7) + adjust);

	// set traits
	for (TraitMap::const_iterator i = traits.begin(); i != traits.end(); ++i) {
		player->set_trait(i->first, i->second);
	}

	// set stats
	for (int i = 0; i < CharStatID::COUNT; ++i)
		player->set_base_stat(i, stats[i]);

	// setup various other bits
	player->recalc();
	player->set_hp(player->get_max_hp());

	// save player
	player->save();

	// add to account
	account->add_character(player->get_id());
	account->save();
	Log::Info << "Character '" << player->get_id() << "' added to account '" << account->get_id() << "'";

	// go to continue state
	enter_state(STATE_CONTINUE);
}

// RETURN TO THE MAIN MENU
void TelnetModeNewCharacter::finish ()
{
	get_handler()->set_mode(new TelnetModeMainMenu(get_handler(), account));
}
