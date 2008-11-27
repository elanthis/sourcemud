/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common/rand.h"
#include "mud/player.h"
#include "mud/login.h"
#include "mud/creation.h"
#include "mud/race.h"
#include "net/telnet.h"

// STAT ROLLING CONFIGURATION
const int STAT_TOKENS = CreatureStatID::COUNT; // how many 'tokens' the player can spend on stats
const int STAT_POINTS = STAT_TOKENS * 25; // how many stat points the player gets randomly assigned
const int STAT_RAND_INC = 5; // the increment by which stats are advanced during random phase
const int STAT_TOKEN_INC = 5; // the increment by which stats are advanced during token phase
const int STAT_BASE = 30; // base value of all stats, before point distribution
const int STAT_MAX = 90; // maximum stat can be raised to in character creation

class TelnetModeRealNewCharacter : public TelnetModeNewCharacter
{
	public:
	enum state_t {
		STATE_BEGIN = 0,
		STATE_NAME = 0,
		STATE_NAME_CONFIRM,
		STATE_RACE,
		STATE_RACE_CONFIRM,
		STATE_GENDER,
		STATE_HEIGHT,
		STATE_BUILD,
		STATE_SKINCOLOR,
		STATE_EYECOLOR,
		STATE_HAIRCOLOR,
		STATE_HAIRSTYLE,
		STATE_FORM_CONFIRM,
		STATE_STATS,
		STATE_STATS_CONFIRM,
		STATE_FINAL_CONFIRM,
		STATE_CONTINUE,
		STATE_RENAME,
		STATE_RENAME_CONFIRM
	};

	TelnetModeRealNewCharacter (TelnetHandler* s_handler, Account* s_account) : TelnetModeNewCharacter (s_handler), account(s_account), state(STATE_BEGIN) {}

	virtual int initialize ();
	virtual void prompt ();
	virtual void process (char* line);
	virtual void shutdown ();
	virtual void finish ();

	private:
	void display ();
	void create ();
	void show_error (std::string msg);
	void enter_state (state_t state);

	static bool is_match (std::string test, std::string operand);

	Account* account;
	std::string name;
	state_t state;
	int tokens;
	int stats[CreatureStatID::COUNT];
	Race* race;
	GenderType gender;
	FormBuild build;
	FormHeight height;
	FormColor eye_color;
	FormColor hair_color;
	FormColor skin_color;
	FormHairStyle hair_style;
};

TelnetModeNewCharacter*
TelnetModeNewCharacter::create (TelnetHandler* handler, Account* account)
{
	return new TelnetModeRealNewCharacter (handler, account);
}

bool TelnetModeRealNewCharacter::is_match (std::string test, std::string operand)
{
	return !operand.empty() && !strncasecmp(test.c_str(), operand.c_str(), operand.size());
}

int TelnetModeRealNewCharacter::initialize ()
{
	// clear all variables
	name.clear();

	// begin
	enter_state(STATE_BEGIN);
	return 0;
}

void TelnetModeRealNewCharacter::shutdown ()
{
}

// DISPLAY CURRENT STATE'S PROMPT
void TelnetModeRealNewCharacter::prompt ()
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
		case STATE_BUILD:
			*get_handler() << "Choose thy physical build:";
			break;
		case STATE_SKINCOLOR:
			*get_handler() << "Choose thy skin color:";
			break;
		case STATE_EYECOLOR:
			*get_handler() << "Choose thy eye color:";
			break;
		case STATE_HAIRCOLOR:
			*get_handler() << "Choose thy hair color:";
			break;
		case STATE_HAIRSTYLE:
			*get_handler() << "Choose thy hair style:";
			break;
		case STATE_STATS:
			*get_handler() << "Which attribute shall thee increase?";
			break;
		case STATE_NAME_CONFIRM:
		case STATE_RACE_CONFIRM:
		case STATE_FORM_CONFIRM:
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
void TelnetModeRealNewCharacter::process (char* line)
{
	std::string input = strlower(S(line));
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
			if (input.empty() || is_match(S("yes"), input))
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
			if (input.empty() || is_match(S("yes"), input))
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
			for (int i = 1; i < FormHeight::COUNT; ++i) {
				if (i == numeric || is_match(FormHeight(i).get_name(), input)) {
					height = FormHeight(i);
					enter_state(STATE_BUILD);
					return;
				}
			}
			show_error(S("I do not understand thy response."));
			break;
		case STATE_BUILD:
			for (int i = 1; i < FormBuild::COUNT; ++i) {
				if (i == numeric || is_match(FormBuild(i).get_name(), input)) {
					build = FormBuild(i);
					enter_state(STATE_SKINCOLOR);
					return;
				}
			}
			show_error(S("I do not understand thy response."));
			break;
		case STATE_SKINCOLOR:
			for (std::vector<FormColor>::const_iterator i = race->get_skin_colors().begin(); i != race->get_skin_colors().end(); ++i) {
				if ((--numeric) == 0 || is_match(i->get_name(), input)) {
					skin_color = *i;
					enter_state(STATE_EYECOLOR);
					return;
				}
			}
			show_error(S("I do not understand thy response."));
			break;
		case STATE_EYECOLOR:
			for (std::vector<FormColor>::const_iterator i = race->get_eye_colors().begin(); i != race->get_eye_colors().end(); ++i) {
				if ((--numeric) == 0 || is_match(i->get_name(), input)) {
					eye_color = *i;
					enter_state(STATE_HAIRCOLOR);
					return;
				}
			}
			show_error(S("I do not understand thy response."));
			break;
		case STATE_HAIRCOLOR:
			for (std::vector<FormColor>::const_iterator i = race->get_hair_colors().begin(); i != race->get_hair_colors().end(); ++i) {
				if ((--numeric) == 0 || is_match(i->get_name(), input)) {
					hair_color = *i;
					enter_state(STATE_HAIRSTYLE);
					return;
				}
			}
			show_error(S("I do not understand thy response."));
			break;
		case STATE_HAIRSTYLE:
			for (int i = 1; i < FormHairStyle::COUNT; ++i) {
				if (i == numeric || is_match(FormHairStyle(i).get_name(), input)) {
					hair_style = FormHairStyle(i);
					enter_state(STATE_FORM_CONFIRM);
					return;
				}
			}
			show_error(S("I do not understand thy response."));
	  		break;
		case STATE_FORM_CONFIRM:
			if (input.empty() || is_match(S("yes"), input))
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
			if (numeric >= 1 && numeric <= CreatureStatID::COUNT) {
				stat = numeric - 1;
			} else {
				for (stat = 0; stat < CreatureStatID::COUNT; ++stat) {
					if (is_match(CreatureStatID(stat).get_name(), input))
						break;
				}
				if (stat == CreatureStatID::COUNT) {
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
			if (input.empty() || is_match(S("yes"), input))
				enter_state(STATE_FINAL_CONFIRM);
			else if (is_match(S("no"), input))
				enter_state(STATE_STATS);
			else
				show_error(S("I do not understand thy response."));
			break;
		case STATE_FINAL_CONFIRM:
			if (input.empty() || is_match(S("yes"), input)) {
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

void TelnetModeRealNewCharacter::display ()
{
	get_handler()->clear_scr();
	*get_handler() << "Character Creation\n------------------\n";
	if (!name.empty()) {
		if (race)
			*get_handler() << name << " (" << capwords(race->get_name()) << ")\n";
		else
			*get_handler() << name << "\n";
	}
	if (gender.get_value() != 0) {
		*get_handler() << capwords(gender.get_name());
		if (height.valid())
			*get_handler() << ", " << height.get_name();
		if (build.valid())
			*get_handler() << ", " << build.get_name();
		*get_handler() << "\n";
	}
	if (skin_color.valid()) {
		*get_handler() << "Skin: " << skin_color.get_name();
		if (eye_color.valid())
			*get_handler() << ", eyes: " << eye_color.get_name();
		if (hair_color.valid()) {
			*get_handler() << ", hair: " << hair_color.get_name();
			if (hair_style.valid())
				*get_handler() << " (" << hair_style.get_name() << ")";
		}
		*get_handler() << "\n";
	}
	if (state > STATE_STATS_CONFIRM) {
		for (int i = 0; i < CreatureStatID::COUNT; ++i) {
			if (i > 0)
				*get_handler() << ", ";
			*get_handler() << CreatureStatID(i).get_short_name() << ':' << stats[i];
		}
		*get_handler() << '\n';
	}

	*get_handler() << "\n";

	switch (state) {
		case STATE_NAME:
			*get_handler() << "Choose the name thy new charater shalle be called.\n\n";
			break;
		case STATE_RENAME:
			*get_handler() << "Thy chosen name has been taken by another.  Choose a new name.\n\n";
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
			for (int i = 0; i < CreatureStatID::COUNT; ++i) {
				if (race->get_stat(i) != 0) {
					*get_handler() << CreatureStatID(i).get_name() << ':';
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
		case STATE_BUILD:
			for (size_t i = 1; i < FormBuild::COUNT; ++i)
				*get_handler() << i << ") " << capwords(FormBuild(i).get_name()) << "\n";
			*get_handler() << "\n";
			break;
		case STATE_SKINCOLOR:
		{
			size_t c = 1;
			for (std::vector<FormColor>::const_iterator i = race->get_skin_colors().begin(); i != race->get_skin_colors().end(); ++i)
				*get_handler() << c++ << ") " << capwords(i->get_name()) << "\n";
			*get_handler() << "\n";
			break;
		}
		case STATE_EYECOLOR:
		{
			size_t c = 1;
			for (std::vector<FormColor>::const_iterator i = race->get_eye_colors().begin(); i != race->get_eye_colors().end(); ++i)
				*get_handler() << c++ << ") " << capwords(i->get_name()) << "\n";
			*get_handler() << "\n";
			break;
		}
		case STATE_HAIRCOLOR:
		{
			size_t c = 1;
			for (std::vector<FormColor>::const_iterator i = race->get_hair_colors().begin(); i != race->get_hair_colors().end(); ++i)
				*get_handler() << c++ << ") " << capwords(i->get_name()) << "\n";
			*get_handler() << "\n";
			break;
		}
		case STATE_HAIRSTYLE:
			for (size_t i = 1; i < FormHairStyle::COUNT; ++i)
				*get_handler() << i << ") " << capwords(FormHairStyle(i).get_name()) << "\n";
			*get_handler() << "\n";
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

			for (int i = 0; i < CreatureStatID::COUNT; ++i) {
				int mod = (stats[i] - 50) / 10;

				// name
				*get_handler() << "  " << (i + 1) << ") " << CreatureStatID(i).get_name() << ": ";

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
			for (size_t i = 1; i < FormHeight::COUNT; ++i)
				*get_handler() << i << ") " << capwords(FormHeight(i).get_name()) << "\n";
			*get_handler() << "\n";
			break;
		case STATE_FINAL_CONFIRM:
		case STATE_RENAME_CONFIRM:
			*get_handler() << "Is it thy wish for me to create this profile?\n\n";
			break;
		case STATE_CONTINUE:
			*get_handler() << " " CADMIN "** Character Created **" CNORMAL "\n\n";
			break;
		default:
			break;
	}
}

void TelnetModeRealNewCharacter::show_error (std::string msg)
{
	display();
	*get_handler() << CWARNING << msg << CNORMAL << "\n\n";
}

void TelnetModeRealNewCharacter::enter_state (state_t new_state)
{
	state = new_state;

	switch (state) {
		case STATE_NAME:
			name.clear();
			break;
		case STATE_RACE:
			race = NULL;
			break;
		case STATE_BUILD:
			// clear all traits
			break;
		case STATE_STATS:
		{
			tokens = STAT_TOKENS;
			for (int i = 0; i < CreatureStatID::COUNT; ++i)
				stats[i] = STAT_BASE;
			for (int i = STAT_POINTS; i > 0; i -= STAT_RAND_INC)
				stats[get_random(CreatureStatID::COUNT)] += STAT_RAND_INC;
			break;
		}
		default:
			break;
	}

	display();
}

void TelnetModeRealNewCharacter::create ()
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

	// set form
	player->set_height(height);
	player->set_build(build);
	player->set_eye_color(eye_color);
	player->set_skin_color(skin_color);
	player->set_hair_color(hair_color);
	player->set_hair_style(hair_style);

	// set stats
	for (int i = 0; i < CreatureStatID::COUNT; ++i)
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
void TelnetModeRealNewCharacter::finish ()
{
	get_handler()->set_mode(new TelnetModeMainMenu(get_handler(), account));
}
