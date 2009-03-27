/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_LOGIN_H
#define SOURCEMUD_MUD_LOGIN_H 1

#include "mud/account.h"
#include "mud/pconn.h"
#include "net/telnet.h"

class TelnetModeLogin : public ITelnetMode
{
public:
	TelnetModeLogin(TelnetHandler* s_handler) : ITelnetMode(s_handler), account(), pass(false), tries(0) {}

	virtual int initialize();
	virtual void prompt();
	virtual void process(char* line);
	virtual void shutdown();

private:
	std::tr1::shared_ptr<Account> account;
	bool pass;
	int tries;
};

class TelnetModeNewAccount : public ITelnetMode
{
	enum { STATE_ID, STATE_NAME, STATE_EMAIL, STATE_PASS, STATE_CHECKPASS, STATE_APPROVE };

public:
	inline TelnetModeNewAccount(TelnetHandler* s_handler) : ITelnetMode(s_handler) {}

	virtual int initialize();
	inline virtual void shutdown() {}
	virtual void process(char* line);
	virtual void prompt();

private:
	std::string id;
	std::string name;
	std::string email;
	std::string passphrase;
	int state;

	void showInfo();
};

class TelnetModeMainMenu : public ITelnetMode
{
	enum { STATE_MENU, STATE_PLAY_SELECT, STATE_DELETE_SELECT,
	       STATE_DELETE_CONFIRM, STATE_ACCOUNT, STATE_CHANGE_EMAIL,
	       STATE_CHANGE_NAME, STATE_CHPASS_CHALLENGE, STATE_CHPASS_SELECT,
	       STATE_CHPASS_CONFIRM
	     };

public:
	inline TelnetModeMainMenu(TelnetHandler* s_handler, std::tr1::shared_ptr<Account> s_account) : ITelnetMode(s_handler), account(s_account), state(STATE_MENU) {}

	virtual int initialize();
	inline virtual void shutdown() {}
	virtual void process(char* line);
	virtual void prompt();

private:
	std::tr1::shared_ptr<Account> account;
	int state;

	// temps
	std::string tmp;

	// menus
	void showBanner();
	void showMain();
	void showCharacters();
	void showCreate();
	void showDelConfirm();
	void showAccount();
};

class TelnetModePlay : public ITelnetMode, public IPlayerConnection
{
public:
	TelnetModePlay(TelnetHandler* s_handler, class Player* s_player) : ITelnetMode(s_handler), player(s_player) {}

	virtual int initialize();
	virtual void prompt();
	virtual void process(char* line);
	virtual void shutdown();
	virtual void finish();

	virtual void pconnConnect(Player* player) {}
	virtual void pconnDisconnect();
	virtual void pconnWrite(const char* data, size_t len) { getHandler()->streamPut(data, len); }
	virtual void pconnSetEcho(bool value) { getHandler()->toggleEcho(value); }
	virtual void pconnSetIndent(uint level) { getHandler()->setIndent(level); }
	virtual void pconnSetColor(int color, int value) { getHandler()->setColor(color, value); }
	virtual void pconnClear() { getHandler()->clearScreen(); }
	virtual void pconnForcePrompt() { getHandler()->forceUpdate(); }
	virtual uint pconnGetWidth() { return getHandler()->getWidth(); }

private:
	class Player* player;
};

#endif
