/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/streams.h"
#include "common/string.h"
#include "mud/creature.h"
#include "mud/server.h"
#include "mud/player.h"
#include "mud/macro.h"
#include "mud/message.h"
#include "mud/account.h"
#include "mud/settings.h"
#include "mud/login.h"
#include "net/telnet.h"

// --- ACCOUNT CREATION ---

int TelnetModeNewAccount::initialize()
{
	showInfo();
	state = STATE_ID;
	return 0;
}

void TelnetModeNewAccount::showInfo()
{
	getHandler()->clearScreen();
	*getHandler() << "Account Information\n";
	*getHandler() << "-------------------\n";

	if (!id.empty())
		*getHandler() << "Account name:   " CPLAYER << id << CNORMAL "\n";
	if (!name.empty())
		*getHandler() << "Real name:      " << name << "\n";
	if (!email.empty())
		*getHandler() << "E-mail address: " << email << "\n";

	*getHandler() << "\n";
}

void TelnetModeNewAccount::prompt()
{
	switch (state) {
	case STATE_ID:
		*getHandler() << "Enter a name for your account:";
		break;
	case STATE_NAME:
		*getHandler() << "Enter your full, *real life* name:";
		break;
	case STATE_EMAIL:
		*getHandler() << "Enter your e-mail address:";
		break;
	case STATE_PASS:
		*getHandler() << "Enter a passphrase:";
		break;
	case STATE_CHECKPASS:
		*getHandler() << "Retype your passphrase:";
		break;
	case STATE_APPROVE:
		*getHandler() << "Is this correct? (Y/n)";
		break;
	default:
		*getHandler() << "Oops, internal error.";
		break;
	}
}

void TelnetModeNewAccount::process(char* data)
{
	std::string line(data);

	switch (state) {
		// select an ID
	case STATE_ID:
		// valid name?
		if (!MAccount.validName(line)) {
			*getHandler() << "\n" CADMIN "Account names must be between " << ACCOUNT_NAME_MIN_LEN << " and " << ACCOUNT_NAME_MAX_LEN << " characters, and consist of only letters and numbers." CNORMAL "\n";
			break;
		}

		// already exists?
		if (MAccount.exists(line)) {
			*getHandler() << "\n" CADMIN "The account name '" << line << "' is already in use." CNORMAL "\n";
		}

		// next
		id = line;
		state = STATE_NAME;
		showInfo();
		break;
		// enter real name
	case STATE_NAME:
		if (!line.empty())
			name = line;

		if (!name.empty()) {
			state = STATE_EMAIL;
			showInfo();
		}
		break;
		// enter email address
	case STATE_EMAIL:
		if (!line.empty()) {
			if (strIsEmail(line)) {
				email = line;
				getHandler()->toggleEcho(false);
			} else {
				email.clear();
				*getHandler() << CADMIN "That is not a valid e-mail address." CNORMAL "\n";
			}
		}

		if (!email.empty()) {
			state = STATE_PASS;
			showInfo();
		}
		break;
		// enter passphrase
	case STATE_PASS:
		// legal?
		if (line.empty() || !MAccount.validPassphrase(line)) {
			*getHandler() << "\n" CADMIN "Passphrases must be at least " << ACCOUNT_PASS_MIN_LEN << " characters, and have both letters and numbers.  Passphrases may also contain symbols or punctuation characters." CNORMAL "\n";
			break;
		}

		// alright, next
		passphrase = line;
		state = STATE_CHECKPASS;
		showInfo();
		break;
		// double check passphrase
	case STATE_CHECKPASS:
		if (passphrase != line) {
			*getHandler() << "\n" CADMIN "Passwords do not match." CNORMAL "\n";
			state = STATE_PASS;
		} else {
			state = STATE_APPROVE;
			showInfo();
			getHandler()->toggleEcho(true);
		}
		break;
		// approve it all
	case STATE_APPROVE:
		// done?
		if (line.empty() || strIsTrue(line)) {
			// double check account is unique
			if (MAccount.get(id)) {
				*getHandler() << "\n" CADMIN "The account name '" << id << "' is already in use." CNORMAL "\n";
				state = STATE_ID;
			} else {
				// create the account!
				std::tr1::shared_ptr<Account> account = MAccount.create(id);
				account->setName(name);
				account->setEmail(email);
				account->setPassphrase(passphrase);

				// enter main menu
				getHandler()->setMode(new TelnetModeMainMenu(getHandler(), account));
				return;
			}
		} else if (strIsFalse(line)) {
			state = STATE_ID;
			showInfo();
		}
		break;
	}
}

// --- LOGIN ---

int TelnetModeLogin::initialize()
{
	return 0;
}

void TelnetModeLogin::prompt()
{
	if (!pass) *getHandler() << "Enter thy name:";
	else *getHandler() << "Enter thy passphrase:";
}

void TelnetModeLogin::process(char* line)
{
	std::string data(line);

	// NUL process command?
	if (data.empty())
		return;

	// quit?
	if (strEq(data, "quit")) {
		getHandler()->disconnect();
		return;
	}

	// not at passphrase stage?
	if (!pass) {
		// no name?
		if (data.empty()) {
			// enabled?
			if (MSettings.getAccountCreation()) {
				*getHandler() << "\nYou must enter your account name to login or type " CBOLD "new" CNORMAL " to begin creating a new account.\n\n";
			} else {
				*getHandler() << "\nYou must enter your account name to login.\n\n";
			}
			return;
		}

		// create account?
		if (strEq(data, "new") || strEq(data, "create")) {
			// enabled?
			if (MSettings.getAccountCreation()) {
				getHandler()->setMode(new TelnetModeNewAccount(getHandler()));
			} else {
				*getHandler() << "\nNew account creation is disabled.\n\n";
			}
			return;
		}

		// invalid name?
		if (!MAccount.validName(data)) {
			*getHandler() << "\nAccount names must be between " << ACCOUNT_NAME_MIN_LEN << " and " << ACCOUNT_NAME_MAX_LEN << " characters, and consist of only letters and numbers.\n\n";
			return;
		}

		// get account
		if (MAccount.exists(data))
			account = MAccount.get(data);

		// do passphrase stage
		pass = true;
		getHandler()->toggleEcho(false);
		// at passphrase stage
	} else {
		getHandler()->toggleEcho(true);
		*getHandler() << "\n";

		// check the account and passphrase
		if (account == NULL || !account->checkPassphrase(data)) {
			*getHandler() << "\nIncorrect account name or passphrase.\n\n";

			// too many failed attempts?
			tries ++;
			if (tries >= 3) {
				*getHandler() << "\n" CADMIN "Too many login failures: disconnecting." CNORMAL "\n";
				Log::Info << "Disconnecting user due to 3 failed login attempts.\n";
				getHandler()->disconnect();
				return;
			}

			// back to login name stage
			account.reset();
			pass = false;
			return;
		}

		// double check stuffs
		if (account->isDisabled()) {
			*getHandler() << "\n" CADMIN "Your account has been disabled by an administrator." CNORMAL "\n";
			Log::Info << "Disabled account '" << account->getId() << "' attempted to login";
			pass = false;
			account.reset();
			return;
		}

		// set login time
		account->updateTimeLogin();

		// ok, do login
		getHandler()->setMode(new TelnetModeMainMenu(getHandler(), account));
	}
}

void TelnetModeLogin::shutdown()
{
	account.reset();
}
