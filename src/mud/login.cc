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

int
TelnetModeNewAccount::initialize()
{
	show_info();
	state = STATE_ID;
	return 0;
}

void
TelnetModeNewAccount::show_info()
{
	get_handler()->clear_scr();
	*get_handler() << "Account Information\n";
	*get_handler() << "-------------------\n";

	if (!id.empty())
		*get_handler() << "Account name:   " CPLAYER << id << CNORMAL "\n";
	if (!name.empty())
		*get_handler() << "Real name:      " << name << "\n";
	if (!email.empty())
		*get_handler() << "E-mail address: " << email << "\n";

	*get_handler() << "\n";
}

void
TelnetModeNewAccount::prompt()
{
	switch (state) {
	case STATE_ID:
		*get_handler() << "Enter a name for your account:";
		break;
	case STATE_NAME:
		*get_handler() << "Enter your full, *real life* name:";
		break;
	case STATE_EMAIL:
		*get_handler() << "Enter your e-mail address:";
		break;
	case STATE_PASS:
		*get_handler() << "Enter a passphrase:";
		break;
	case STATE_CHECKPASS:
		*get_handler() << "Retype your passphrase:";
		break;
	case STATE_APPROVE:
		*get_handler() << "Is this correct? (Y/n)";
		break;
	default:
		*get_handler() << "Oops, internal error.";
		break;
	}
}

void
TelnetModeNewAccount::process(char* data)
{
	std::string line(data);

	switch (state) {
		// select an ID
	case STATE_ID:
		// valid name?
		if (!MAccount.validName(line)) {
			*get_handler() << "\n" CADMIN "Account names must be between " << ACCOUNT_NAME_MIN_LEN << " and " << ACCOUNT_NAME_MAX_LEN << " characters, and consist of only letters and numbers." CNORMAL "\n";
			break;
		}

		// already exists?
		if (MAccount.exists(line)) {
			*get_handler() << "\n" CADMIN "The account name '" << line << "' is already in use." CNORMAL "\n";
		}

		// next
		id = line;
		state = STATE_NAME;
		show_info();
		break;
		// enter real name
	case STATE_NAME:
		if (!line.empty())
			name = line;

		if (!name.empty()) {
			state = STATE_EMAIL;
			show_info();
		}
		break;
		// enter email address
	case STATE_EMAIL:
		if (!line.empty()) {
			if (str_is_email(line)) {
				email = line;
				get_handler()->toggle_echo(false);
			} else {
				email.clear();
				*get_handler() << CADMIN "That is not a valid e-mail address." CNORMAL "\n";
			}
		}

		if (!email.empty()) {
			state = STATE_PASS;
			show_info();
		}
		break;
		// enter passphrase
	case STATE_PASS:
		// legal?
		if (line.empty() || !MAccount.validPassphrase(line)) {
			*get_handler() << "\n" CADMIN "Passphrases must be at least " << ACCOUNT_PASS_MIN_LEN << " characters, and have both letters and numbers.  Passphrases may also contain symbols or punctuation characters." CNORMAL "\n";
			break;
		}

		// alright, next
		passphrase = line;
		state = STATE_CHECKPASS;
		show_info();
		break;
		// double check passphrase
	case STATE_CHECKPASS:
		if (passphrase != line) {
			*get_handler() << "\n" CADMIN "Passwords do not match." CNORMAL "\n";
			state = STATE_PASS;
		} else {
			state = STATE_APPROVE;
			show_info();
			get_handler()->toggle_echo(true);
		}
		break;
		// approve it all
	case STATE_APPROVE:
		// done?
		if (line.empty() || str_is_true(line)) {
			// double check account is unique
			if (MAccount.get(id)) {
				*get_handler() << "\n" CADMIN "The account name '" << id << "' is already in use." CNORMAL "\n";
				state = STATE_ID;
			} else {
				// create the account!
				std::tr1::shared_ptr<Account> account = MAccount.create(id);
				account->setName(name);
				account->setEmail(email);
				account->setPassphrase(passphrase);

				// enter main menu
				get_handler()->set_mode(new TelnetModeMainMenu(get_handler(), account));
				return;
			}
		} else if (str_is_false(line)) {
			state = STATE_ID;
			show_info();
		}
		break;
	}
}

// --- LOGIN ---

int
TelnetModeLogin::initialize()
{
	return 0;
}

void
TelnetModeLogin::prompt()
{
	if (!pass) *get_handler() << "Enter thy name:";
	else *get_handler() << "Enter thy passphrase:";
}

void
TelnetModeLogin::process(char* line)
{
	std::string data(line);

	// NUL process command?
	if (data.empty())
		return;

	// quit?
	if (str_eq(data, "quit")) {
		get_handler()->disconnect();
		return;
	}

	// not at passphrase stage?
	if (!pass) {
		// no name?
		if (data.empty()) {
			// enabled?
			if (MSettings.get_account_creation()) {
				*get_handler() << "\nYou must enter your account name to login or type " CBOLD "new" CNORMAL " to begin creating a new account.\n\n";
			} else {
				*get_handler() << "\nYou must enter your account name to login.\n\n";
			}
			return;
		}

		// create account?
		if (str_eq(data, "new") || str_eq(data, "create")) {
			// enabled?
			if (MSettings.get_account_creation()) {
				get_handler()->set_mode(new TelnetModeNewAccount(get_handler()));
			} else {
				*get_handler() << "\nNew account creation is disabled.\n\n";
			}
			return;
		}

		// invalid name?
		if (!MAccount.validName(data)) {
			*get_handler() << "\nAccount names must be between " << ACCOUNT_NAME_MIN_LEN << " and " << ACCOUNT_NAME_MAX_LEN << " characters, and consist of only letters and numbers.\n\n";
			return;
		}

		// get account
		if (MAccount.exists(data))
			account = MAccount.get(data);

		// do passphrase stage
		pass = true;
		get_handler()->toggle_echo(false);
		// at passphrase stage
	} else {
		get_handler()->toggle_echo(true);
		*get_handler() << "\n";

		// check the account and passphrase
		if (account == NULL || !account->checkPassphrase(data)) {
			*get_handler() << "\nIncorrect account name or passphrase.\n\n";

			// too many failed attempts?
			tries ++;
			if (tries >= 3) {
				*get_handler() << "\n" CADMIN "Too many login failures: disconnecting." CNORMAL "\n";
				Log::Info << "Disconnecting user due to 3 failed login attempts.\n";
				get_handler()->disconnect();
				return;
			}

			// back to login name stage
			account.reset();
			pass = false;
			return;
		}

		// double check stuffs
		if (account->isDisabled()) {
			*get_handler() << "\n" CADMIN "Your account has been disabled by an administrator." CNORMAL "\n";
			Log::Info << "Disabled account '" << account->getId() << "' attempted to login";
			pass = false;
			account.reset();
			return;
		}

		// set login time
		account->updateTimeLogin();

		// ok, do login
		get_handler()->set_mode(new TelnetModeMainMenu(get_handler(), account));
	}
}

void
TelnetModeLogin::shutdown()
{
	account.reset();
}
