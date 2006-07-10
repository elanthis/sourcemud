/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_LOGIN_H
#define AWEMUD_MUD_LOGIN_H 1

#include "mud/telnet.h"
#include "mud/account.h"

class TelnetModeLogin : public ITelnetMode
{
	public:
	TelnetModeLogin (TelnetHandler* s_handler) : ITelnetMode (s_handler), account(NULL), pass(false), tries(0) {}

	virtual int initialize ();
	virtual void prompt ();
	virtual void process (char* line);
	virtual void shutdown ();

	private:
	class Account* account;
	bool pass;
	int tries;
};

class TelnetModeNewAccount : public ITelnetMode
{
	enum { STATE_ID, STATE_NAME, STATE_EMAIL, STATE_PASS, STATE_CHECKPASS, STATE_APPROVE };

	public:
	inline TelnetModeNewAccount (TelnetHandler* s_handler) : ITelnetMode (s_handler) {}

	virtual int initialize ();
	inline virtual void shutdown () {}
	virtual void process (char* line);
	virtual void prompt ();

	private:
	String id;
	String name;
	String email;
	String passphrase;
	int state;

	void show_info ();
};

class TelnetModeMainMenu : public ITelnetMode
{
	enum { STATE_MENU, STATE_PLAY_SELECT, STATE_DELETE_SELECT,
		STATE_DELETE_CONFIRM, STATE_ACCOUNT, STATE_CHANGE_EMAIL,
		STATE_CHANGE_NAME, STATE_CHPASS_CHALLENGE, STATE_CHPASS_SELECT,
		STATE_CHPASS_CONFIRM };

	public:
	inline TelnetModeMainMenu (TelnetHandler* s_handler, Account* s_account) : ITelnetMode(s_handler), account(s_account), state(STATE_MENU) {}

	virtual int initialize ();
	inline virtual void shutdown () {}
	virtual void process (char* line);
	virtual void prompt ();

	private:
	Account* account;
	int state;

	// temps
	String tmp;

	// menus
	void show_banner ();
	void show_main ();
	void show_chars ();
	void show_create ();
	void show_del_confirm ();
	void show_account ();
};

class TelnetModePlay : public ITelnetMode
{
	public:
	TelnetModePlay (TelnetHandler* s_handler, class Player* s_player) : ITelnetMode (s_handler), player(s_player) {}

	virtual int initialize ();
	virtual void prompt ();
	virtual void process (char* line);
	virtual void shutdown ();
	virtual void finish ();

	private:
	class Player* player;
};

#endif
