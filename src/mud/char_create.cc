/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/account.h"
#include "mud/player.h"
#include "mud/race.h"
#include "mud/telnet.h"
#include "common/rand.h"

// --- CONFIGURATION ---
	// how many 'tokens' the player can spend on stats
	const int STAT_TOKENS = 6;
	// how many stat points the player gets randomly assigned
	const int STAT_POINTS = 160;
	// the increment by which stats are advanced during random phase
	const int STAT_RAND_INC = 2;
	// the increment by which stats are advanced during token phase
	const int STAT_TOKEN_INC = 5;
	// base value of all stats, before point distribution
	const int STAT_BASE = 30;
	// maximum stat can be raised to in character creation
	const int STAT_MAX = 90;
// --- END ---

class CharacterCreationProcessor : public IProcessor
{
	public:
	enum {
		STATE_RACE = 0,
		STATE_RACE_CONFIRM,
		STATE_ALIGN,
		STATE_STATS,
		STATE_STATS_CONFIRM,
		STATE_GENDER,
		STATE_TRAITS,
		STATE_HEIGHT,
		STATE_DETAIL_CONFIRM,
		STATE_FINAL_CONFIRM,
		STATE_DONE,
	};

	CharacterCreationProcessor (Player* player);

	// BASICS
	virtual int init ();
	virtual void finish ();
	virtual int process (char* line);
	virtual const char* prompt ();

	// CORE
	void display ();
	void show_player_info ();
	void start_state (int new_state);
	void show_player_stats ();
	void set_prompt (StringArg str);
	Race* lookup_race (StringArg input);
	void show_races ();
	void show_genders ();
	void show_traits ();
	void display_race (Race* race);
	void display_details ();
	void roll_stats ();

	private:
	String the_prompt;
	int tokens;
	int state;
	Race* race;
};

int
CharacterCreationProcessor::init ()
{
	start_state(0); // begin state machine

	return 0;
}

CharacterCreationProcessor::CharacterCreationProcessor (Player* player) :
	IProcessor(player),
	the_prompt(),
	tokens(0),
	state(0), // <-- first state
	race(NULL)
{}

void
CharacterCreationProcessor::start_state (int state)
{
	if (state == STATE_STATS) { // stats
		// tokens
		tokens = STAT_TOKENS;
		roll_stats();
	} else if (state == STATE_TRAITS) { // traits
		start_state(STATE_HEIGHT); return;
	/* FIXME
		trait = -1;
		nextTrait();
		if (trait == -1)
			startState(state + 1);
			*/
	} else if (state == STATE_FINAL_CONFIRM) { // finish
		// set birthday
		/* FIXME
		int birthdate = getTime();
		birthdate.setYear (birthdate.getYear() - player->get_race().getRandAge());
		player->setBirthday(birthdate);
		*/

		// give 'em some coins
		player->give_coins(500);

		// finish final calculations
		player->recalc();
		player->set_hp(player->get_max_hp());
	}

	display();
}

void CharacterCreationProcessor::display ()
{
	player->get_telnet()->clear_scr();
	*player << 	"Character Creation\n"
		"------------------\n"
		"Name: " << player << "\n\n";
	
	if (state == STATE_RACE) { // race
		*player << 
			"Your race, or species, defines what kind of being you are.  "
			"Different races have different physical, mental, and "
			"spiritual strengths.\n\n";
		show_races();
		set_prompt("Select race:");
	} else if (state == STATE_RACE_CONFIRM) {
		display_race(race);
		set_prompt("Keep race? (y/N)");
	} else if (state == STATE_GENDER) { // gender
		show_genders();
		set_prompt("Select gender:");
	} else if (state == STATE_TRAITS) {
	/* FIXME
		show_traits();
		set_prompt("Select " :: all_traits[trait] :: ":");
		*/
	} else if (state == STATE_HEIGHT) { // height
		*player << StreamParse("The average height for a {$1.gender} {$1.race} is ", "player", player) << player->get_race()->get_average_height(player->get_gender()) << " inches, give or take three.\n\n";
		*player << "Height Adjustment:\n";
		*player << "  1) Very short (12 inches shorter)\n";
		*player << "  2) Short (6 inches shorter)\n";
		*player << "  3) Average (normal average)\n";
		*player << "  4) Tall (6 inches taller)\n";
		*player << "  5) Very tall (12 inches taller)\n\n";
		set_prompt("Select height:");
	} else if (state == STATE_ALIGN) { // alignment
		*player << 
			"Your alignment is an indication of your moral and ethical "
			"beliefs.  Good characters believe in doing what is right and "
			"just, even at their own expense.  Evil characters will often "
			"use excessively cruel tactics to reach their goals.  Neutral "
			"characters are in the middle of the two extremes.\n\n";
		*player << "Alignments:\n";
		*player << "  1) Good\n";
		*player << "  2) Neutral\n";
		*player << "  3) Evil\n\n";
		set_prompt("Select alignment:");
	} else if (state == STATE_STATS) { // stats
		*player << 
			"You have " << tokens << " tokens with which to increase your "
			"character attributes.  Each token increases an attribute "
			"by " << STAT_TOKEN_INC << " points.  Your base attributes are "
			"determined randomly; all characters will have the same total "
			"of attribute points, however.  You may at any time reroll "
			"your stats by typing 'reroll'.\n\n";
		show_player_stats();
		set_prompt("Increase stat:");
	} else if (state == STATE_STATS_CONFIRM) {
		show_player_stats();
		set_prompt("Keep stats? (Y/n)");
	} else if (state == STATE_DETAIL_CONFIRM) {
		display_details();
		set_prompt("Keep details? (Y/n)");
	} else if (state == STATE_FINAL_CONFIRM) {
		show_player_info();
		*player << "\n";
		set_prompt("Accept new character? (Y/n)");
	}
}

int CharacterCreationProcessor::process (char* input)
{
	String line = strlower(input);

	// reset handler?
	if (line == "restart") {
		*player << "\n\nRestarting character creation.";
		state = 0;
		start_state(0);
		return 0;
	}

	// quit or exit?
	if (line == "quit" || line == "exit") {
		return 1;
	}

	// new state
	int state = this->state;

	// update 
	if (state == STATE_RACE) { // race
		// help?
		if (phrase_match(line, "help")) {
			display();
			// FIXME  player->exec("help charcreate-race");
			return 0;
		}

		// lookup race
		race = lookup_race(line);
		if (race) {
			++state;
		} else {
			display();
		}
	} else if (state == STATE_RACE_CONFIRM) { // confirm
		// said yes?
		int choice = str_is_true(line);
		if (choice > 0) {
			player->set_race(race);
			++state;
		} else if (!line || choice < 0) { // default
			// back to race selection
			state = STATE_RACE;
		}
	} else if (state == STATE_GENDER) { // gender
		int gender = tolong(line);
		// check gender
		if (gender == GenderType::MALE || gender == GenderType::FEMALE) {
			player->set_gender(gender);
			++state;
		} else if (phrase_match(line, "female")) {
			player->set_gender(GenderType::FEMALE);
			++state;
		} else if (phrase_match(line, "male")) {
			player->set_gender(GenderType::MALE);
			++state;
		} else {
			display();
		}
	} else if (state == STATE_TRAITS) { // select traits
		/* FIXME
		int trait = tolong(line) - 1;
		if (trait < 0 || trait >= cur_traits.length()) {
			display();
			return 0;
		}

		player->setTrait(all_traits[trait], cur_traits[trait]);
		nextTrait();
		if (trait == -1)
			++state;
		else
			display();
			*/
	} else if (state == STATE_HEIGHT) { // player height
		// help?
		if (phrase_match(line, "help")) {
			display();
			// FIXME player->exec("help charcreate-height");
			return 0;
		}

		int adjust = 0;
		int ok = 0;

		int num = tolong(line);
		if (num == 2 || phrase_match(line, "short")) {
			adjust = -6;
			ok = 1;
		} else if (num == 1 || phrase_match(line, "very short")) {
			adjust = -12;
			ok = 1;
		} else if (num == 4 || phrase_match(line, "tall")) {
			adjust = 6;
			ok = 1;
		} else if (num == 5 || phrase_match(line, "very tall")) {
			adjust = 12;
			ok = 1;
		} else if (num == 3 || phrase_match(line, "average")) {
			adjust = 0;
			ok = 1;
		} else {
			display();
		}

		if (ok) {
			++state;

			int height = player->get_race()->get_average_height(player->get_gender()) - 3 + get_random(7) + adjust;
			player->set_height(height);
		}

	} else if (state == STATE_ALIGN) { // alignment
		// help?
		if (phrase_match(line, "help")) {
			display();
			// FIXME player->exec("help charcreate-align");
			return 0;
		}

		int num = tolong(line);
		if (num == 1 || phrase_match(line, "good")) { // good
			player->set_alignment(400);
			++state;
		} else if (num == 2 || phrase_match(line, "neutral")) { // neutral
			player->set_alignment(0);
			++state;
		} else if (num == 3 || phrase_match(line, "evil")) { // evil
			player->set_alignment(-400);
			++state;
		} else {
			display();
		}
	} else if (state == STATE_STATS) { // stats
		// help?
		if (phrase_match(line, "help")) {
			display();
			// FIXME: player->exec("help charcreate-stats");
			return 0;
		}

		// empty line?
		if (!line) {
			display();
			return 0;
		}

		// reroll?
		if (line == "roll" || line == "reroll") {
			// slight hack
			this->state = STATE_STATS;
			start_state(STATE_STATS);
			return 0;
		}

		// check stat
		int stat = tolong(line) - 1;
		if (stat < 0 || stat >= CharStatID::COUNT) {
			// not a number, a name?
			for (stat = 0; stat < CharStatID::COUNT; ++stat) {
				if (phrase_match(line, CharStatID(stat).get_name()))
					break;
			}
			// didn't find a named stat
			if (stat == CharStatID::COUNT) {
				display();
				return 0;
			}
		}

		// stat maxed out?
		if (stats[stat] >= STAT_MAX) {
			display();
			*player << "You cannot increase that statistic any further.\n\n";
			return 0;
		}

		// do work
		player->set_base_stat (stat, player->get_base_stat (stat) + STAT_TOKEN_INC);
		tokens = tokens - 1;
		player->recalc_stats();

		// out of tokens?
		if (tokens == 0) {
			state = STATE_STATS_CONFIRM;
		} else {
			display();
			*player << "You have " << tokens << " tokens left.\n\n";
		}
	} else if (state == STATE_STATS_CONFIRM) { // confirm
		// said yes?
		int choice = str_is_true(line);
		if (!line || choice > 0) { // default
			++state;
		} else if (choice < 0) {
			state = STATE_STATS;
		} else {
			display();
		}
	} else if (state == STATE_DETAIL_CONFIRM) { // confirm details
		// said yes?
		int choice = str_is_true(line);
		if (!line || choice > 0) { // default
			++state;
		} else if (choice < 0) {
			// back to detail selection
			state = STATE_GENDER;
		}
	} else if (state == STATE_FINAL_CONFIRM) { // confirm character
		// said yes?
		int choice = str_is_true(line);
		if (!line || choice > 0) { // default
			++state;
		} else if (choice < 0) {
			// start over
			state = 0;
		}
	}

	// all done, there is no more
	if (state == STATE_DONE) {
		// make valid
		player->validate();

		// exit
		return 1;
	// next state
	} else if (state != this->state) {
		start_state(state);
	}

	return 0;
}


#if 0

	// select next valid trait
	nextTrait()
	{
		// next trait; increment
		// FIXME: trait++ gives 'not a structure' error... odd
		trait = trait + 1;

		// no more?
		if (trait >= all_traits.length()) {
			trait = -1;
			return;
		}

		// get traits and number of traits
		cur_traits = race.getTraitValues(all_traits[trait]);
		int count = cur_traits.length();

		// race has none for this trait?  skip
		if (count == 0) {
			nextTrait();
			return;
		}

		// race has only for value for this trait?  set and skip
		if (count == 1) {
			player->setTrait(all_traits[trait], cur_traits[0]);
			nextTrait();
			return;
		}
	}
}
#endif

void CharacterCreationProcessor::set_prompt (StringArg string)
{
	the_prompt = string;
}

Race* CharacterCreationProcessor::lookup_race (StringArg input)
{
	if (str_is_number(input)) {
		long index = tolong(input);
		Race* race = RaceManager.first();
		while (index-- != 0 && race != NULL)
			race = race->get_next();
		return race;
	} else {
		Race* race = RaceManager.first();
		while (race != NULL && !phrase_match(race->get_name(), input))
			race = race->get_next();
		return race;
	}
}

void CharacterCreationProcessor::display_race (Race* race) {
	*player << "Race: " << race->get_name() << "\n";
	player->set_indent(2);

	// Stats
	for (int i = 0; i < CharStatID::COUNT; ++i) {
		if (race->get_stat(i) != 0) {
			*player << CharStatID(i).get_name() << ':';
			player->set_indent(16);
			if (race->get_stat(i) > 0)
				*player << '+';
			*player << race->get_stat(i) << "\n";
			player->set_indent(2);
		}
	}

	// Lifespan
	*player << "Life span:";
	player->set_indent(16);
	*player << race->get_life_span() << " years\n";

	// About
	player->set_indent(0);
	*player << "\n" << CDESC << race->get_about() << CNORMAL << "\n\n";
}

void CharacterCreationProcessor::show_races()
{
	*player << "Available races:\n";
	player->set_indent(2);
	int index = 1;
	Race* race = RaceManager.first();
	while (race != NULL) {
		*player << (index++) << ") " << capwords(race->get_name()) << "\n";
		race = race->get_next();
	}
	player->set_indent(0);
}

void CharacterCreationProcessor::show_genders()
{
	*player << "Available genders:\n";
	*player << "  1) Female\n";	// GenderType::FEMALE=1
	*player << "  2) Male\n";	// GenderType::MALE=2
}

void CharacterCreationProcessor::show_traits()
{
/* FIXME
	*player << "Available ", all_traits[trait], " choices:\n");
	int num = 1;
	foreach (int trait in cur_traits)
		*player << "  " << num++ << ") " << capwords(trait) << "\n");
*/
}

void CharacterCreationProcessor::display_details ()
{
	*player << "Description:\n  " << StreamParse("{$1.Desc}", "player", player) << "\n\n";
}

void CharacterCreationProcessor::roll_stats ()
{
	int points = STAT_POINTS;

	// initialize
	for (int i = 0; i < CharStatID::COUNT; ++ i)
		player->set_base_stat(i, STAT_BASE);

	// distribute randomness
	while ((points -= STAT_RAND_INC) > 0) {
		int stat = get_random(CharStatID::COUNT);
		player->set_base_stat(stat, player->get_base_stat(stat) + STAT_RAND_INC);
	}

	// recalc
	player->recalc_stats();
}

void CharacterCreationProcessor::show_player_stats ()
{
	*player << "Your statistics:\n";
	for (int i = 0; i < CharStatID::COUNT; ++i) {
		int stat = player->get_effective_stat(i);
		int mod = player->get_stat_modifier(i);

		// name
		*player << "  " << CharStatID(i).get_name() << ": ";

		// descriptor
		player->set_indent(14);
		*player << get_stat_color(stat) << get_stat_level(stat) << CNORMAL;

		// value/+mod
		player->set_indent(25);
		*player << '(' << stat << '/';
		if (mod >= 0)
			*player << '+';
		*player << mod << ")\n";

		player->set_indent(0);
	}
}

void CharacterCreationProcessor::show_player_info ()
{
}

void CharacterCreationProcessor::finish ()
{
}

const char* CharacterCreationProcessor::prompt ()
{
	return the_prompt.c_str();
}

// 'create' the chracter
int Player::create (void)
{
	assert(!flags.valid);

	IProcessor* creation = new CharacterCreationProcessor(this);
	add_processor(creation);

	return 0;
}

// make player valid
int Player::validate (void)
{
	// already valid?  silly dink.
	if (is_valid())
		return -1;

	// add character to account
	get_account()->add_character(get_id());
	get_account()->save();

	// set as valid
	flags.valid = true;

	// save
	save();

	// log it
	Log::Info << "New player " << get_id() << " created for account " << get_account()->get_id();

	// start the player
	return start();
}
