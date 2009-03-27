/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/rand.h"
#include "common/string.h"
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

	TelnetModeRealNewCharacter(TelnetHandler* s_handler, std::tr1::shared_ptr<Account> s_account) : TelnetModeNewCharacter(s_handler), account(s_account), state(STATE_BEGIN) {}

	virtual int initialize();
	virtual void prompt();
	virtual void process(char* line);
	virtual void shutdown();
	virtual void finish();

private:
	void display();
	void create();
	void show_error(const std::string& msg);
	void enter_state(state_t state);

	static bool is_match(const std::string& test, const std::string& operand);

	std::tr1::shared_ptr<Account> account;
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

TelnetModeNewCharacter* TelnetModeNewCharacter::create(TelnetHandler* handler, std::tr1::shared_ptr<Account> account)
{
	return new TelnetModeRealNewCharacter(handler, account);
}

bool TelnetModeRealNewCharacter::is_match(const std::string& test, const std::string& operand)
{
	return !operand.empty() && !strncasecmp(test.c_str(), operand.c_str(), operand.size());
}

int TelnetModeRealNewCharacter::initialize()
{
	// clear all variables
	name.clear();

	// begin
	enter_state(STATE_BEGIN);
	return 0;
}

void TelnetModeRealNewCharacter::shutdown()
{
}

// DISPLAY CURRENT STATE'S PROMPT
void TelnetModeRealNewCharacter::prompt()
{
	switch (state) {
	case STATE_NAME:
	case STATE_RENAME:
		*getHandler() << "Enter thy name:";
		break;
	case STATE_RACE:
		*getHandler() << "Choose thy race:";
		break;
	case STATE_GENDER:
		*getHandler() << "Choose thy gender:";
		break;
	case STATE_HEIGHT:
		*getHandler() << "Choose thy stature:";
		break;
	case STATE_BUILD:
		*getHandler() << "Choose thy physical build:";
		break;
	case STATE_SKINCOLOR:
		*getHandler() << "Choose thy skin color:";
		break;
	case STATE_EYECOLOR:
		*getHandler() << "Choose thy eye color:";
		break;
	case STATE_HAIRCOLOR:
		*getHandler() << "Choose thy hair color:";
		break;
	case STATE_HAIRSTYLE:
		*getHandler() << "Choose thy hair style:";
		break;
	case STATE_STATS:
		*getHandler() << "Which attribute shall thee increase?";
		break;
	case STATE_NAME_CONFIRM:
	case STATE_RACE_CONFIRM:
	case STATE_FORM_CONFIRM:
	case STATE_STATS_CONFIRM:
	case STATE_FINAL_CONFIRM:
	case STATE_RENAME_CONFIRM:
		*getHandler() << "Accept? (Y/n)";
		break;
	case STATE_CONTINUE:
		*getHandler() << "Thy profile is complete. (Press ENTER to continue)";
		break;
	}
}

// PROCESS INPUT FOR CURRENT STATE
void TelnetModeRealNewCharacter::process(char* line)
{
	std::string input = strlower(line);
	int numeric = tolong(input);

	if (input == "quit") {
		finish();
		return;
	}

	switch (state) {
	case STATE_NAME:
	case STATE_RENAME:
		// must be a valid name
		if (!MPlayer.validName(input)) {
			show_error("Thy chosen name is not acceptable.");
			break;
		}

		// not already in use
		if (MPlayer.exists(input)) {
			show_error("Thy chosen name is already in use.");
			break;
		}

		// go to confirmation
		name = capwords(input);
		enter_state(state == STATE_NAME ? STATE_NAME_CONFIRM : STATE_RENAME_CONFIRM);
		break;
	case STATE_NAME_CONFIRM:
		if (input.empty() || is_match("yes", input))
			enter_state(state == STATE_NAME_CONFIRM ? STATE_RACE : STATE_FINAL_CONFIRM);
		else if (is_match("no", input))
			enter_state(STATE_NAME);
		else
			show_error("I do not understand thy response.");
		break;
	case STATE_RACE: {
		// find the selected race
		Race* rptr = MRace.first();
		int index = 1;
		int selected = numeric;
		while (rptr != NULL) {
			if (phraseMatch(rptr->getName(), input) || index == selected)
				break;
			rptr = rptr->getNext();
			++ index;
		}
		if (rptr == NULL) {
			show_error("I do not understand thy response.");
			break;
		}

		// set race, confirm
		race = rptr;
		enter_state(STATE_RACE_CONFIRM);
		break;
	}
	case STATE_RACE_CONFIRM:
		if (input.empty() || is_match("yes", input))
			enter_state(STATE_GENDER);
		else if (is_match("no", input))
			enter_state(STATE_RACE);
		else
			show_error("I do not understand thy response.");
		break;
	case STATE_GENDER:
		if (numeric == 1 || is_match("female", input)) {
			gender = GenderType::FEMALE;
			enter_state(STATE_HEIGHT);
		} else if (numeric == 2 || is_match("male", input)) {
			gender = GenderType::MALE;
			enter_state(STATE_HEIGHT);
		} else {
			show_error("I do not understand thy response.");
		}
		break;
	case STATE_HEIGHT:
		for (int i = 1; i < FormHeight::COUNT; ++i) {
			if (i == numeric || is_match(FormHeight(i).getName(), input)) {
				height = FormHeight(i);
				enter_state(STATE_BUILD);
				return;
			}
		}
		show_error("I do not understand thy response.");
		break;
	case STATE_BUILD:
		for (int i = 1; i < FormBuild::COUNT; ++i) {
			if (i == numeric || is_match(FormBuild(i).getName(), input)) {
				build = FormBuild(i);
				enter_state(STATE_SKINCOLOR);
				return;
			}
		}
		show_error("I do not understand thy response.");
		break;
	case STATE_SKINCOLOR:
		for (std::vector<FormColor>::const_iterator i = race->getSkinColors().begin(); i != race->getSkinColors().end(); ++i) {
			if ((--numeric) == 0 || is_match(i->getName(), input)) {
				skin_color = *i;
				enter_state(STATE_EYECOLOR);
				return;
			}
		}
		show_error("I do not understand thy response.");
		break;
	case STATE_EYECOLOR:
		for (std::vector<FormColor>::const_iterator i = race->getEyeColors().begin(); i != race->getEyeColors().end(); ++i) {
			if ((--numeric) == 0 || is_match(i->getName(), input)) {
				eye_color = *i;
				enter_state(STATE_HAIRCOLOR);
				return;
			}
		}
		show_error("I do not understand thy response.");
		break;
	case STATE_HAIRCOLOR:
		for (std::vector<FormColor>::const_iterator i = race->getHairColors().begin(); i != race->getHairColors().end(); ++i) {
			if ((--numeric) == 0 || is_match(i->getName(), input)) {
				hair_color = *i;
				enter_state(STATE_HAIRSTYLE);
				return;
			}
		}
		show_error("I do not understand thy response.");
		break;
	case STATE_HAIRSTYLE:
		for (int i = 1; i < FormHairStyle::COUNT; ++i) {
			if (i == numeric || is_match(FormHairStyle(i).getName(), input)) {
				hair_style = FormHairStyle(i);
				enter_state(STATE_FORM_CONFIRM);
				return;
			}
		}
		show_error("I do not understand thy response.");
		break;
	case STATE_FORM_CONFIRM:
		if (input.empty() || is_match("yes", input))
			enter_state(STATE_STATS);
		else if (is_match("no", input))
			enter_state(STATE_GENDER); // redo gender and height, too
		else
			show_error("I do not understand thy response.");
		break;
	case STATE_STATS: {
		// reroll?
		if (input == "reroll" || input == "reset") {
			enter_state(STATE_STATS);
			break;
		}

		// determine chosen stat
		int stat;
		if (numeric >= 1 && numeric <= CreatureStatID::COUNT) {
			stat = numeric - 1;
		} else {
			for (stat = 0; stat < CreatureStatID::COUNT; ++stat) {
				if (is_match(CreatureStatID(stat).getName(), input))
					break;
			}
			if (stat == CreatureStatID::COUNT) {
				show_error("I do not understand thy response.");
				break;
			}
		}

		// make sure stat is under limit
		if (stats[stat] >= STAT_MAX) {
			show_error("Ye may not increase that attribute any further.");
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
		if (input.empty() || is_match("yes", input))
			enter_state(STATE_FINAL_CONFIRM);
		else if (is_match("no", input))
			enter_state(STATE_STATS);
		else
			show_error("I do not understand thy response.");
		break;
	case STATE_FINAL_CONFIRM:
		if (input.empty() || is_match("yes", input)) {
			if (MPlayer.exists(name))
				enter_state(STATE_RENAME);
			else
				create();
		} else if (is_match("no", input)) {
			enter_state(STATE_BEGIN);
		} else {
			show_error("I do not understand thy response.");
		}
		break;
	default:
		finish();
		break;
	}
}

void TelnetModeRealNewCharacter::display()
{
	getHandler()->clearScreen();
	*getHandler() << "Character Creation\n------------------\n";
	if (!name.empty()) {
		if (race)
			*getHandler() << name << " (" << capwords(race->getName()) << ")\n";
		else
			*getHandler() << name << "\n";
	}
	if (gender.getValue() != 0) {
		*getHandler() << capwords(gender.getName());
		if (height.valid())
			*getHandler() << ", " << height.getName();
		if (build.valid())
			*getHandler() << ", " << build.getName();
		*getHandler() << "\n";
	}
	if (skin_color.valid()) {
		*getHandler() << "Skin: " << skin_color.getName();
		if (eye_color.valid())
			*getHandler() << ", eyes: " << eye_color.getName();
		if (hair_color.valid()) {
			*getHandler() << ", hair: " << hair_color.getName();
			if (hair_style.valid())
				*getHandler() << " (" << hair_style.getName() << ")";
		}
		*getHandler() << "\n";
	}
	if (state > STATE_STATS_CONFIRM) {
		for (int i = 0; i < CreatureStatID::COUNT; ++i) {
			if (i > 0)
				*getHandler() << ", ";
			*getHandler() << CreatureStatID(i).getShortName() << ':' << stats[i];
		}
		*getHandler() << '\n';
	}

	*getHandler() << "\n";

	switch (state) {
	case STATE_NAME:
		*getHandler() << "Choose the name thy new charater shalle be called.\n\n";
		break;
	case STATE_RENAME:
		*getHandler() << "Thy chosen name has been taken by another.  Choose a new name.\n\n";
		break;
	case STATE_RACE: {
		Race* rptr = MRace.first();
		int index = 1;
		while (rptr != NULL) {
			*getHandler() << index << ") " << capwords(rptr->getName()) << "\n";
			rptr = rptr->getNext();
			++ index;
		}
		*getHandler() << "\n";
		break;
	}
	case STATE_RACE_CONFIRM: {
		getHandler()->setIndent(2);
		for (int i = 0; i < CreatureStatID::COUNT; ++i) {
			if (race->getStat(i) != 0) {
				*getHandler() << CreatureStatID(i).getName() << ':';
				getHandler()->setIndent(16);
				if (race->getStat(i) > 0)
					*getHandler() << '+';
				else if (race->getStat(i) < 0)
					*getHandler() << '-';
				else
					*getHandler() << ' ';
				if (abs(race->getStat(i)) < 10)
					*getHandler() << ' ';
				*getHandler() << abs(race->getStat(i)) << "\n";
				getHandler()->setIndent(2);
			}
		}
		getHandler()->setIndent(0);

		*getHandler() << "\nLife span: ";
		*getHandler() << race->getLifeSpan() << " years\n";

		// About
		*getHandler() << "\n" << CDESC << race->getAbout() << CNORMAL << "\n\n";
		break;
	}
	case STATE_GENDER:
		*getHandler() << "1) Female\n2) Male\n\n";
		break;
	case STATE_BUILD:
		for (size_t i = 1; i < FormBuild::COUNT; ++i)
			*getHandler() << i << ") " << capwords(FormBuild(i).getName()) << "\n";
		*getHandler() << "\n";
		break;
	case STATE_SKINCOLOR: {
		size_t c = 1;
		for (std::vector<FormColor>::const_iterator i = race->getSkinColors().begin(); i != race->getSkinColors().end(); ++i)
			*getHandler() << c++ << ") " << capwords(i->getName()) << "\n";
		*getHandler() << "\n";
		break;
	}
	case STATE_EYECOLOR: {
		size_t c = 1;
		for (std::vector<FormColor>::const_iterator i = race->getEyeColors().begin(); i != race->getEyeColors().end(); ++i)
			*getHandler() << c++ << ") " << capwords(i->getName()) << "\n";
		*getHandler() << "\n";
		break;
	}
	case STATE_HAIRCOLOR: {
		size_t c = 1;
		for (std::vector<FormColor>::const_iterator i = race->getHairColors().begin(); i != race->getHairColors().end(); ++i)
			*getHandler() << c++ << ") " << capwords(i->getName()) << "\n";
		*getHandler() << "\n";
		break;
	}
	case STATE_HAIRSTYLE:
		for (size_t i = 1; i < FormHairStyle::COUNT; ++i)
			*getHandler() << i << ") " << capwords(FormHairStyle(i).getName()) << "\n";
		*getHandler() << "\n";
		break;
	case STATE_STATS:
	case STATE_STATS_CONFIRM: {
		*getHandler() <<
		"You have " << STAT_TOKENS << " tokens with which to increase your "
		"character attributes.  Each token increases an attribute "
		"by " << STAT_TOKEN_INC << " points.  Your base attributes are "
		"determined randomly; all characters will have the same total "
		"of attribute points, however.  You may at any time reroll "
		"your stats by typing 'reroll'.\n\n";

		for (int i = 0; i < CreatureStatID::COUNT; ++i) {
			int mod = (stats[i] - 50) / 10;

			// name
			*getHandler() << "  " << (i + 1) << ") " << CreatureStatID(i).getName() << ": ";

			// descriptor
			getHandler()->setIndent(14);
			*getHandler() << getStatColor(stats[i]) << getStatLevel(stats[i]) << CNORMAL;

			// value/+mod
			getHandler()->setIndent(25);
			*getHandler() << '(' << stats[i] << '/';
			if (mod >= 0)
				*getHandler() << '+';
			*getHandler() << mod << ")\n";

			getHandler()->setIndent(0);
		}

		// display remaining tokens
		if (tokens > 0)
			*getHandler() << "\nYe have " << tokens << " tokens remaining.\n\n";
		else
			*getHandler() << "\nThy tokens have been spent.\n\n";
		break;
	}
	case STATE_HEIGHT:
		for (size_t i = 1; i < FormHeight::COUNT; ++i)
			*getHandler() << i << ") " << capwords(FormHeight(i).getName()) << "\n";
		*getHandler() << "\n";
		break;
	case STATE_FINAL_CONFIRM:
	case STATE_RENAME_CONFIRM:
		*getHandler() << "Is it thy wish for me to create this profile?\n\n";
		break;
	case STATE_CONTINUE:
		*getHandler() << " " CADMIN "** Character Created **" CNORMAL "\n\n";
		break;
	default:
		break;
	}
}

void TelnetModeRealNewCharacter::show_error(const std::string& msg)
{
	display();
	*getHandler() << CWARNING << msg << CNORMAL << "\n\n";
}

void TelnetModeRealNewCharacter::enter_state(state_t new_state)
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
	case STATE_STATS: {
		tokens = STAT_TOKENS;
		for (int i = 0; i < CreatureStatID::COUNT; ++i)
			stats[i] = STAT_BASE;
		for (int i = STAT_POINTS; i > 0; i -= STAT_RAND_INC)
			stats[Random::get(CreatureStatID::COUNT)] += STAT_RAND_INC;
		break;
	}
	default:
		break;
	}

	display();
}

void TelnetModeRealNewCharacter::create()
{
	Player* player;

	// create the player
	player = new Player(account, name);

	// set basics
	player->setRace(race);
	player->setGender(gender);

	// set birthday
	int age = race->getAgeMin() + Random::get(race->getAgeMax() - race->getAgeMin());
	GameTime birthday = MTime.time;
	birthday.setYear(birthday.getYear() - age);
	player->setBirthday(birthday);

	// set form
	player->setHeight(height);
	player->setBuild(build);
	player->setEyeColor(eye_color);
	player->setSkinColor(skin_color);
	player->setHairColor(hair_color);
	player->setHairStyle(hair_style);

	// set stats
	for (int i = 0; i < CreatureStatID::COUNT; ++i)
		player->setBaseStat(i, stats[i]);

	// setup various other bits
	player->recalc();
	player->setHP(player->getMaxHP());

	// save player
	player->save();

	// add to account
	account->addCharacter(player->getId());
	account->save();
	Log::Info << "Character '" << player->getId() << "' added to account '" << account->getId() << "'";

	// go to continue state
	enter_state(STATE_CONTINUE);
}

// RETURN TO THE MAIN MENU
void TelnetModeRealNewCharacter::finish()
{
	getHandler()->setMode(new TelnetModeMainMenu(getHandler(), account));
}
