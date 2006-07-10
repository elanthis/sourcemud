/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_CREATION_H
#define AWEMUD_MUD_CREATION_H 1

#include "mud/telnet.h"
#include "mud/account.h"
#include "mud/player.h"
#include "mud/race.h"

class TelnetModeNewCharacter : public ITelnetMode
{
	public:
	enum state_t { STATE_BEGIN = 0, STATE_NAME = 0, STATE_NAME_CONFIRM, STATE_RACE, STATE_RACE_CONFIRM, STATE_GENDER, STATE_HEIGHT, STATE_TRAITS, STATE_TRAITS_CONFIRM, STATE_STATS, STATE_STATS_CONFIRM, STATE_FINAL_CONFIRM, STATE_CONTINUE, STATE_RENAME, STATE_RENAME_CONFIRM
	};
	enum height_t { HEIGHT_VERY_SHORT, HEIGHT_SHORT, HEIGHT_AVERAGE, HEIGHT_TALL, HEIGHT_VERY_TALL };

	TelnetModeNewCharacter (TelnetHandler* s_handler, Account* s_account) : ITelnetMode (s_handler), account(s_account), state(STATE_BEGIN) {}

	virtual int initialize ();
	virtual void prompt ();
	virtual void process (char* line);
	virtual void shutdown ();
	virtual void finish ();

	void display ();
	void show_error (StringArg msg);
	void enter_state (state_t state);

	void create ();

	static bool is_match (StringArg test, StringArg operand);

	private:
	Account* account;
	String name;
	state_t state;
	height_t height;
	int tokens;
	int stats[CharStatID::COUNT];
	Race* race;
	GenderType gender;
	typedef GCType::map<CharacterTraitID, CharacterTraitValue> TraitMap;
	TraitMap traits;
};

#endif
