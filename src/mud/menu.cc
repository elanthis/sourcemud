/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <ctype.h>

#include "mud/char.h"
#include "mud/server.h"
#include "mud/player.h"
#include "mud/parse.h"
#include "common/streams.h"
#include "mud/message.h"
#include "mud/telnet.h"
#include "mud/account.h"
#include "mud/settings.h"
#include "mud/login.h"

// --- MAIN MENU ----

int
TelnetModeMainMenu::initialize (void)
{
	// set timeout to account's value
	if (account->get_timeout() != 0)
		get_handler()->set_timeout(account->get_timeout());

	// show menu
	show_main();
	return 0;
}

void
TelnetModeMainMenu::show_banner (void)
{
	get_handler()->clear_scr();
	*get_handler() << StreamParse(MessageManager.get("menu_banner")) << "\n";
	*get_handler() << "Greetings, " CPLAYER << account->get_name() << CNORMAL "!\n\n";
}

void
TelnetModeMainMenu::show_main (void)
{
	show_banner();

	*get_handler() << " 1) Play\n";
	*get_handler() << " 2) Create new character\n";
	*get_handler() << " 3) Account details\n";
	*get_handler() << " 4) Delete character\n";
	*get_handler() << " 5) Quit\n";

	*get_handler() << "\n";
}

void
TelnetModeMainMenu::show_chars (void)
{
	show_banner();

	*get_handler() << "Your available characaters:\n\n";

	if (account->get_char_list().empty()) {
		*get_handler() << " 1) Return to main menu\n";
		*get_handler() << "\nYou do not have any characters created yet.\n";
	} else {
		uint i = 0;
		for (; i < account->get_char_list().size(); ++i)
			*get_handler() << " " << (i + 1) << ") " CPLAYER << account->get_char_list()[i] << CNORMAL "\n";
		*get_handler() << " " << (i + 1) << ") Return to main menu\n";
	}


	*get_handler() << "\n";
}

void
TelnetModeMainMenu::show_account (void)
{
	show_banner();

	*get_handler() << " 1) Name:   " << account->get_name() << "\n";
	*get_handler() << " 2) E-Mail: " << account->get_email() << "\n";
	*get_handler() << " 3) Passphrase\n";
	*get_handler() << " 4) Return to main menu\n";

	*get_handler() << "\n";
}

void
TelnetModeMainMenu::prompt (void)
{
	switch (state) {
		case STATE_MENU: *get_handler() << "Select:"; break;
		case STATE_PLAY_SELECT: *get_handler() << "Character to play:"; break;
		case STATE_CREATE_SELECT: *get_handler() << "New character's name:"; break;
		case STATE_DELETE_SELECT: *get_handler() << "Character to delete:"; break;
		case STATE_DELETE_CONFIRM: *get_handler() << "Confirm:"; break;
		case STATE_ACCOUNT: *get_handler() << "Select:"; break;
		case STATE_CHANGE_NAME: *get_handler() << "Your real-life name:"; break;
		case STATE_CHANGE_EMAIL: *get_handler() << "Your e-mail address:"; break;
		case STATE_CHPASS_CHALLENGE: *get_handler() << "Your current passphrase:"; break;
		case STATE_CHPASS_SELECT: *get_handler() << "New passphrase:"; break;
		case STATE_CHPASS_CONFIRM: *get_handler() << "Confirm passphrase:"; break;
	}
}

void
TelnetModeMainMenu::show_create (void)
{
	show_banner();

	*get_handler() << "Enter the name of your new character, or type " CBOLD "return" CNORMAL " to return to the main menu.\n\n";
}

void
TelnetModeMainMenu::process (char* line)
{
	switch (state) {
		case STATE_MENU:
			// erm, nothing?
			if (!strlen(line)) {
				show_main();
			// play?
			} else if (str_eq("1", line) || phrase_match("play", line)) {
				state = STATE_PLAY_SELECT;
				show_chars();
			// create?
			} else if (str_eq("2", line) || phrase_match("create", line)) {
				// check max
				if (account->get_char_list().size() >= account->get_max_chars()) {
					show_main();
					*get_handler() << "You already have the maximum number of characters allowed.\n\n";
				} else {
					show_create();
					state = STATE_CREATE_SELECT;
				}
			// account?
			} else if (str_eq("3", line) || phrase_match("account", line)) {
				state = STATE_ACCOUNT;
				show_account();
			// delete?
			} else if (str_eq("4", line) || phrase_match("delete", line)) {
				state = STATE_DELETE_SELECT;
				show_chars();
			// exit?
			} else if (str_eq("5", line) || phrase_match("exit", line)) {
				*get_handler() << StreamParse(MessageManager.get("quit"));
				get_handler()->disconnect();
			// eh?
			} else {
				show_main();
				*get_handler() << "I don't understand.\n\n";
			}
			break;
		case STATE_PLAY_SELECT:
		{
			// conver input
			int inopt = tolong(line) - 1;
			if (inopt == (int)account->get_char_list().size() || phrase_match("return", line)) {
				state = STATE_MENU;
				show_main();
				break;
			}

			// whom did they ask for?
			int choice = -1;
			for (int i = 0; i < (int)account->get_char_list().size(); ++i) {
				if (inopt == i || phrase_match(account->get_char_list()[i], line)) {
					choice = i;
					break;
				}
			}
			if (choice == -1) {
				state = STATE_MENU;
				show_main();
				*get_handler() << "Invalid character.\n\n";
				break;
			}

			// get player
			Player* player = PlayerManager.load(account, account->get_char_list()[choice]);
			if (player == NULL) {
				show_chars();
				*get_handler() << CADMIN "Error: Character cannot be found." CNORMAL "\n\n";
				break;
			}

			// check active limit
			if (!player->is_connected()) {
				uint max_active = account->get_max_active();
				if (account->get_active() >= max_active && !player->is_active()) {
					state = STATE_MENU;
					show_main();
					*get_handler() << "You already have the maximum number of characters active at a time.\n\n";
					break;
				}
			}

			// set play mode
			get_handler()->set_mode(new TelnetModePlay(get_handler(), player));
			break;
		}
		case STATE_CREATE_SELECT:
		{
			// return?
			if (!strlen(line) || phrase_match("return", line)) {
				show_main();
				state = STATE_MENU;
				break;
			}

			// valid?
			if (!PlayerManager.valid_name(line)) {
				show_create();
				*get_handler() << "Character names must be between " << PLAYER_NAME_MIN_LEN << " and " << PLAYER_NAME_MAX_LEN << " characters long, and may consist only of letters.\n\n";
				break;
			}

			// fixup name
			String name;
			capwords(name, line);

			// player already exist?
			if (PlayerManager.exists(name)) {
				show_create();
				*get_handler() << "A character named " CPLAYER << name << CNORMAL " already exists.\n\n";
				break;
			}

			// check max
			if (account->get_char_list().size() >= account->get_max_chars()) {
				state = STATE_MENU;
				show_main();
				*get_handler() << "You already have the maximum number of characters allowed.\n\n";
				break;
			}

			// create
			PlayerManager.create(get_handler(), account);
			break;
		}
		case STATE_ACCOUNT:
			// change name?
			if (str_eq("1", line) || phrase_match("name", line)) {
				state = STATE_CHANGE_NAME;
				show_banner();
				*get_handler() << "Current name: " << account->get_name() << ".\n\n";
				*get_handler() << "You may correct your real-life name registered with this account.  To leave your name unchanged, do not enter and text and simple press return/enter.\n\n";
			// change email?
			} else if (str_eq("2", line) || phrase_match("email", line)) {
				state = STATE_CHANGE_EMAIL;
				show_banner();
				*get_handler() << "Current email address: " << account->get_email() << ".\n\n";
				*get_handler() << "You may correct the email address registered with this account.  To leave your email address unchanged, do not enter and text and simple press return/enter.\n\n";
			// change password?
			} else if (str_eq("3", line) || phrase_match("passphrase", line)) {
				state = STATE_CHPASS_CHALLENGE;
				show_banner();
				*get_handler() << "You must enter your current passphrase before you may select a new one.\n\n";
				get_handler()->toggle_echo(false);
			// return?
			} else if (!strlen(line) || str_eq("4", line) || phrase_match("return", line)) {
				show_main();
				state = STATE_MENU;
			// er?
			} else {
				show_account();
				*get_handler() << "I don't understand.\n\n";
			}
			break;
		case STATE_CHANGE_NAME:
			// return to menu
			state = STATE_ACCOUNT;

			// not empty?  change
			if (strlen(line)) {
				account->set_name(line);
				show_account();
				*get_handler() << "Your name has been changed.\n\n";
			} else {
				show_account();
			}
			break;
		case STATE_CHANGE_EMAIL:
			// return to menu
			state = STATE_ACCOUNT;

			// not empty?  change
			if (strlen(line)) {
				account->set_email(line);
				show_account();
				*get_handler() << "Your email address has been changed.\n\n";
			} else {
				show_account();
			}
			break;
		case STATE_CHPASS_CHALLENGE:
			// check it
			if (!account->check_passphrase(line)) {
				state = STATE_ACCOUNT;
				show_account();
				*get_handler() << "Passphrase incorrect.\n\n";
				get_handler()->toggle_echo(true);
				break;
			}

			// alright, next state
			state = STATE_CHPASS_SELECT;
			show_banner();
			*get_handler() << "You may now enter a new passphrase.\n\n";
			break;
		case STATE_CHPASS_SELECT:
			// must be valid
			if (!strlen(line) || !AccountManager.valid_passphrase(line)) {
				state = STATE_ACCOUNT;
				show_account();
				*get_handler() << "Passphrases must be at least " << ACCOUNT_PASS_MIN_LEN << " characters, and have both letters and numbers.  Passphrases may also contain symbols or punctuation characters.\n\n";
				get_handler()->toggle_echo(true);
				break;
			}

			// store it in temporary
			tmp = line;

			// next state
			state = STATE_CHPASS_CONFIRM;
			show_banner();
			*get_handler() << "You must now retype your passphrase to confirm that you have entered it correctly.\n\n";
			break;
		case STATE_CHPASS_CONFIRM:
			// re-enable echo
			get_handler()->toggle_echo(true);

			// confirm it
			if (strcmp(line, tmp)) {
				state = STATE_ACCOUNT;
				show_account();
				*get_handler() << "Passphrases do not match.\n\n";
				break;
			}


			// set passphrase
			account->set_passphrase(line);

			// clear the tmp passphrase (no, this isn't for security - way too weak for that)
			tmp.clear();

			// all done
			state = STATE_ACCOUNT;
			show_account();
			*get_handler() << "Your account passphrase has been changed.\n\n";
			break;
		case STATE_DELETE_SELECT:
		{
			// conver input
			int inopt = tolong(line) - 1;
			if (inopt == (int)account->get_char_list().size() || phrase_match("return", line)) {
				state = STATE_MENU;
				show_main();
				break;
			}

			// whom did they ask for?
			int choice = -1;
			for (int i = 0; i < (int)account->get_char_list().size(); ++i) {
				if (inopt == i || phrase_match(account->get_char_list()[i], line)) {
					choice = i;
					break;
				}
			}
			if (choice == -1) {
				state = STATE_MENU;
				show_main();
				*get_handler() << "Invalid character.\n\n";
				break;
			}

			// store choice
			tmp = account->get_char_list()[choice];
			
			// go to confirmation
			state = STATE_DELETE_CONFIRM;
			show_banner();
			*get_handler() << "Do you wish to delete " CPLAYER << account->get_char_list()[choice] << CNORMAL "?\n\n";
			*get_handler() << CADMIN "Warning!!" CNORMAL "  If you delete a character, you will " CADMIN "not" CNORMAL " be able to get the character back, ever.\n\n";
			*get_handler() << "To confirm the deletion of this character, you must type " CBOLD "I am sure!" CNORMAL " with that exact spelling, punctuation, and capitalization.  If you do not wish to delete this character, simply press enter/return.\n\n";
			break;
		}
		case STATE_DELETE_CONFIRM:
			// return to menu
			state = STATE_MENU;
			show_main();

			// confirmed?  delete...
			if (!strcmp(line, "I am sure!")) {
				// did delete work?
				if (PlayerManager.destroy(tmp)) {
					*get_handler() << "Internal error; could not delete character.\n\n";
					tmp.clear();
					break;
				}

				// message
				account->del_character(tmp);
				Log::Info << "Account '" << account->get_id() << "' deleted player '" << tmp << "'";
				*get_handler() << "Character " CPLAYER << tmp << CNORMAL " has been permanently deleted.\n\n";
				tmp.clear();
			} else {
				// no delete
				tmp.clear();
				*get_handler() << "Your character has not been deleted.\n\n";
			}
			break;
	}
}

// --- PLAY ---
int
TelnetModePlay::initialize (void)
{
	player->connect(get_handler());

	// start the player
	if (!player->is_valid() || player->start()) {
		*get_handler() << "\n" CADMIN "Failed to start your login session." CNORMAL "\n";
		Log::Warning << "Failed to start login session for '" << player->get_id() << "'.";
		return -1;
	}

	// finish off
	Log::Info << player->get_id() << " logged in";
	return 0;
}

void
TelnetModePlay::prompt (void)
{
	player->show_prompt();
}

void
TelnetModePlay::process (char* line)
{
	player->process_command(line);
}

void
TelnetModePlay::check (void)
{
	if (player->get_telnet() != get_handler()) {
		get_handler()->set_mode(new TelnetModeMainMenu(get_handler(), player->get_account()));
		player = NULL;
		return;
	}
}

void
TelnetModePlay::shutdown (void)
{
	if (player && player->get_telnet() == get_handler()) {
		player->disconnect();
		player = NULL;
	}
}
