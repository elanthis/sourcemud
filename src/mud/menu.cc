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
#include "mud/macro.h"
#include "common/streams.h"
#include "mud/message.h"
#include "mud/telnet.h"
#include "mud/account.h"
#include "mud/settings.h"
#include "mud/login.h"
#include "mud/creation.h"

// --- MAIN MENU ----

int
TelnetModeMainMenu::initialize ()
{
	// set timeout to account's value
	if (account->get_timeout() != 0)
		get_handler()->set_timeout(account->get_timeout());

	// show menu
	show_main();
	return 0;
}

void
TelnetModeMainMenu::show_banner ()
{
	get_handler()->clear_scr();
	*get_handler() << StreamMacro(MessageManager.get(S("menu_banner"))) << S("\n");
	*get_handler() << "Greetings, " CPLAYER << account->get_name() << CNORMAL "!\n\n";
}

void
TelnetModeMainMenu::show_main ()
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
TelnetModeMainMenu::show_characters()
{
	show_banner();

	*get_handler() << "Your available characters:\n\n";

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
TelnetModeMainMenu::show_account ()
{
	show_banner();

	*get_handler() << " 1) Name:   " << account->get_name() << "\n";
	*get_handler() << " 2) E-Mail: " << account->get_email() << "\n";
	*get_handler() << " 3) Passphrase\n";
	*get_handler() << " 4) Return to main menu\n";

	*get_handler() << "\n";
}

void
TelnetModeMainMenu::prompt ()
{
	switch (state) {
		case STATE_MENU: *get_handler() << "Select:"; break;
		case STATE_PLAY_SELECT: *get_handler() << "Character to play:"; break;
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
TelnetModeMainMenu::show_create ()
{
	show_banner();

	*get_handler() << "Enter the name of your new character, or type " CBOLD "return" CNORMAL " to return to the main menu.\n\n";
}

void
TelnetModeMainMenu::process (char* line)
{
	String input(line);

	switch (state) {
		case STATE_MENU:
			// erm, nothing?
			if (input.empty()) {
				show_main();
			// play?
			} else if (input == "1" || prefix_match("play", input)) {
				state = STATE_PLAY_SELECT;
				show_characters();
			// create?
			} else if (input == "2" || prefix_match("create", input)) {
				// check max
				if (account->get_char_list().size() >= account->get_max_characters()) {
					show_main();
					*get_handler() << "You already have the maximum number of characters allowed.\n\n";
				} else {
					// create
					ITelnetMode* mode = new TelnetModeNewCharacter(get_handler(), account);
					if (mode != NULL)
						get_handler()->set_mode(new TelnetModeNewCharacter(get_handler(), account));
				}
			// account?
			} else if (input == "3" || prefix_match("account", input)) {
				state = STATE_ACCOUNT;
				show_account();
			// delete?
			} else if (input == "4" || prefix_match("delete", input)) {
				state = STATE_DELETE_SELECT;
				show_characters();
			// portal?
			} else if (input == "5" || prefix_match("portal", input)) {
				*get_handler() << StreamMacro(MessageManager.get(S("quit")));
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
			int inopt = tolong(input) - 1;
			if (inopt == (int)account->get_char_list().size() || prefix_match("return", input)) {
				state = STATE_MENU;
				show_main();
				break;
			}

			// whom did they ask for?
			int choice = -1;
			for (int i = 0; i < (int)account->get_char_list().size(); ++i) {
				if (inopt == i || phrase_match(account->get_char_list()[i], input)) {
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
				show_characters();
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
		case STATE_ACCOUNT:
			// change name?
			if (input == "1" || prefix_match("name", input)) {
				state = STATE_CHANGE_NAME;
				show_banner();
				*get_handler() << "Current name: " << account->get_name() << ".\n\n";
				*get_handler() << "You may correct your real-life name registered with this account.  To leave your name unchanged, do not enter and text and simple press return/enter.\n\n";
			// change email?
			} else if (input == "2" || prefix_match("email", input)) {
				state = STATE_CHANGE_EMAIL;
				show_banner();
				*get_handler() << "Current email address: " << account->get_email() << ".\n\n";
				*get_handler() << "You may correct the email address registered with this account.  To leave your email address unchanged, do not enter and text and simple press return/enter.\n\n";
			// change password?
			} else if (input == "3" || prefix_match("passphrase", input)) {
				state = STATE_CHPASS_CHALLENGE;
				show_banner();
				*get_handler() << "You must enter your current passphrase before you may select a new one.\n\n";
				get_handler()->toggle_echo(false);
			// return?
			} else if (input.empty() || input == "4" || prefix_match("return", input)) {
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
			if (strlen(input)) {
				account->set_name(input);
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
			if (strlen(input)) {
				account->set_email(input);
				show_account();
				*get_handler() << "Your email address has been changed.\n\n";
			} else {
				show_account();
			}
			break;
		case STATE_CHPASS_CHALLENGE:
			// check it
			if (!account->check_passphrase(input)) {
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
			if (input.empty() || !AccountManager.valid_passphrase(input)) {
				state = STATE_ACCOUNT;
				show_account();
				*get_handler() << "Passphrases must be at least " << ACCOUNT_PASS_MIN_LEN << " characters, and have both letters and numbers.  Passphrases may also contain symbols or punctuation characters.\n\n";
				get_handler()->toggle_echo(true);
				break;
			}

			// store it in temporary
			tmp = input;

			// next state
			state = STATE_CHPASS_CONFIRM;
			show_banner();
			*get_handler() << "You must now retype your passphrase to confirm that you have entered it correctly.\n\n";
			break;
		case STATE_CHPASS_CONFIRM:
			// re-enable echo
			get_handler()->toggle_echo(true);

			// confirm it
			if (strcmp(input, tmp)) {
				state = STATE_ACCOUNT;
				show_account();
				*get_handler() << "Passphrases do not match.\n\n";
				break;
			}


			// set passphrase
			account->set_passphrase(input);

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
			int inopt = tolong(input) - 1;
			if (inopt == (int)account->get_char_list().size() || prefix_match("return", input)) {
				state = STATE_MENU;
				show_main();
				break;
			}

			// whom did they ask for?
			int choice = -1;
			for (int i = 0; i < (int)account->get_char_list().size(); ++i) {
				if (inopt == i || phrase_match(account->get_char_list()[i], input)) {
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
			if (!strcmp(input, "I am sure!")) {
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
TelnetModePlay::initialize ()
{
	player->connect(this);

	// start the player
	if (player->start_session() != 0) {
		*get_handler() << "\n" CADMIN "Failed to start your login session." CNORMAL "\n";
		Log::Warning << "Failed to start login session for '" << player->get_id() << "'.";
		return -1;
	}

	// finish off
	Log::Info << player->get_id() << " logged in";
	return 0;
}

void
TelnetModePlay::pconn_disconnect ()
{
	player = NULL;
	get_handler()->disconnect();
}

void
TelnetModePlay::prompt ()
{
	player->show_prompt();
}

void
TelnetModePlay::process (char* line)
{
	player->process_command(String(line));
}

void
TelnetModePlay::finish ()
{
	// disconnect if necessary
	get_handler()->set_mode(new TelnetModeMainMenu(get_handler(), player->get_account()));
}

void
TelnetModePlay::shutdown ()
{
	if (player && dynamic_cast<TelnetHandler*>(player->get_conn()) == get_handler()) {
		player->disconnect();
		player = NULL;
	}
}
