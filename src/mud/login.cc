/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <ctype.h>

#include "mud/creature.h"
#include "mud/server.h"
#include "mud/player.h"
#include "mud/parse.h"
#include "common/streams.h"
#include "mud/message.h"
#include "mud/telnet.h"
#include "mud/account.h"
#include "mud/settings.h"
#include "mud/login.h"

// --- ACCOUNT CREATION ---

int
TelnetModeNewAccount::initialize (void)
{
	show_info();
	state = STATE_ID;
	return 0;
}

void
TelnetModeNewAccount::show_info (void)
{
	get_handler()->clear_scr();
	*get_handler() << "Account Information\n";
	*get_handler() << "-------------------\n";

	if (id)
		*get_handler() << "Account name:   " CPLAYER << id << CNORMAL "\n";
	if (name)
		*get_handler() << "Real name:      " << name << "\n";
	if (email)
		*get_handler() << "E-mail address: " << email << "\n";

	*get_handler() << "\n";
}

void
TelnetModeNewAccount::prompt (void)
{
	switch (state) {
		case STATE_ID: *get_handler() << "Enter a name for your account:"; break;
		case STATE_NAME: *get_handler() << "Enter your full, *real life* name:"; break;
		case STATE_EMAIL: *get_handler() << "Enter your e-mail address:"; break;
		case STATE_PASS: *get_handler() << "Enter a passphrase:"; break;
		case STATE_CHECKPASS: *get_handler() << "Retype your passphrase:"; break;
		case STATE_APPROVE: *get_handler() << "Is this correct? (Y/n)"; break;
		default: *get_handler() << "Oops, internal error."; break;
	}
}

void
TelnetModeNewAccount::process (char* data)
{
	String line(data);

	switch (state) {
		// select an ID
		case STATE_ID:
			// valid name?
			if (!AccountManager.valid_name(line)) {
				*get_handler() << "\n" CADMIN "Account names must be between " << ACCOUNT_NAME_MIN_LEN << " and " << ACCOUNT_NAME_MAX_LEN << " characters, and consist of only letters and numbers." CNORMAL "\n";
				break;
			}

			// already exists?
			if (AccountManager.exists(line)) {
				*get_handler() << "\n" CADMIN "The account name '" << line << "' is already in use." CNORMAL "\n";
			}

			// next
			id = line;
			state = STATE_NAME;
			show_info();
			break;
		// enter real name
		case STATE_NAME:
			if (strlen(line))
				name = line;

			if (name) {
				state = STATE_EMAIL;
				show_info();
			}
			break;
		// enter email address
		case STATE_EMAIL:
			if (strlen(line)) {
				if (str_is_email(line)) {
					email = line;
					get_handler()->toggle_echo(false);
				} else {
					email.clear();
					*get_handler() << CADMIN "That is not a valid e-mail address." CNORMAL "\n";
				}
			}

			if (email) {
				state = STATE_PASS;
				show_info();
			}
			break;
		// enter passphrase
		case STATE_PASS:
			// legal?
			if (!strlen(line) || !AccountManager.valid_passphrase(line)) {
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
			if (strcmp(passphrase, line)) {
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
			if (!strlen(line) || !strncasecmp (line, "yes", strlen (line))) {
				// double check account is unique
				if (AccountManager.get(id)) {
					*get_handler() << "\n" CADMIN "The account name '" << id << "' is already in use." CNORMAL "\n";
					state = STATE_ID;
				} else {
					// create the account!
					Account* account = AccountManager.create(id);
					account->set_name(name);
					account->set_email(email);
					account->set_passphrase(passphrase);

					// enter main menu
					get_handler()->set_mode(new TelnetModeMainMenu(get_handler(), account));
					return;
				}
			} else if (!strncasecmp(line, "no", strlen(line))) {
				state = STATE_ID;
				show_info();
			}
			break;
	}
}

// --- LOGIN ---

int
TelnetModeLogin::initialize (void)
{
	return 0;
}

void
TelnetModeLogin::prompt (void)
{
	if(!pass) *get_handler() << "Enter thy name:";
	else *get_handler() << "Enter thy passphrase:";
}

void
TelnetModeLogin::process (char* line)
{
	String data(line);

	// NUL process command?
	if (data.empty())
		return;

	// quit?
	if (str_eq(data, S("quit"))) {
		get_handler()->disconnect();
		return;
	}

	// not at passphrase stage?
	if (!pass) {
		// no name?
		if (!strlen(data)) {
			// enabled?
			if (SettingsManager.get_account_creation()) {
				*get_handler() << "\nYou must enter your account name to login or type " CBOLD "new" CNORMAL " to begin creating a new account.\n\n";
			} else {
				*get_handler() << "\nYou must enter your account name to login.\n\n";
			}
			return;
		}

		// create account?
		if (str_eq(data, S("new")) || str_eq(data, S("create"))) {
			// enabled?
			if (SettingsManager.get_account_creation()) {
				get_handler()->set_mode(new TelnetModeNewAccount(get_handler()));
			} else {
				*get_handler() << "\nNew account creation is disabled.\n\n";
			}
			return;
		}

		// invalid name?
		if (!AccountManager.valid_name (data)) {
			*get_handler() << "\nAccount names must be between " << ACCOUNT_NAME_MIN_LEN << " and " << ACCOUNT_NAME_MAX_LEN << " characters, and consist of only letters and numbers.\n\n";
			return;
		}

		// get account
		if (AccountManager.exists(data))
			account = AccountManager.get(data);

		// do passphrase stage
		pass = true;
		get_handler()->toggle_echo (false);
	// at passphrase stage
	} else {
		get_handler()->toggle_echo (true);
		*get_handler() << "\n";

		// check the account and passphrase
		if (account == NULL || !account->check_passphrase(data)) {
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
			account = NULL;
			pass = false;
			return;
		}

		// double check stuffs
		if (account->is_disabled()) {
			*get_handler() << "\n" CADMIN "Your account has been disabled by an administrator." CNORMAL "\n";
			Log::Info << "Disabled account '" << account->get_id() << "' attempted to login";
			pass = false;
			account = NULL;
			return;
		}

		// set login time
		account->update_time_lastlogin();

		// ok, do login
		get_handler()->set_mode(new TelnetModeMainMenu(get_handler(), account));
	}
}

void
TelnetModeLogin::shutdown (void)
{
	account = NULL;
}
