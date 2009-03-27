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
#include "mud/creation.h"
#include "net/telnet.h"

// --- MAIN MENU ----

int TelnetModeMainMenu::initialize()
{
	// set timeout to account's value
	if (account->getTimeout() != 0)
		getHandler()->setTimeout(account->getTimeout());

	// show menu
	showMain();
	return 0;
}

void TelnetModeMainMenu::showBanner()
{
	getHandler()->clearScreen();
	*getHandler() << StreamMacro(MMessage.get("menu_banner")) << "\n";
	*getHandler() << "Greetings, " CPLAYER << account->getName() << CNORMAL "!\n\n";
}

void TelnetModeMainMenu::showMain()
{
	showBanner();

	*getHandler() << " 1) Play\n";
	*getHandler() << " 2) Create new character\n";
	*getHandler() << " 3) Account details\n";
	*getHandler() << " 4) Delete character\n";
	*getHandler() << " 5) Quit\n";

	*getHandler() << "\n";
}

void TelnetModeMainMenu::showCharacters()
{
	showBanner();

	*getHandler() << "Your available characters:\n\n";

	if (account->getCharList().empty()) {
		*getHandler() << " 1) Return to main menu\n";
		*getHandler() << "\nYou do not have any characters created yet.\n";
	} else {
		uint i = 0;
		for (; i < account->getCharList().size(); ++i)
			*getHandler() << " " << (i + 1) << ") " CPLAYER << account->getCharList()[i] << CNORMAL "\n";
		*getHandler() << " " << (i + 1) << ") Return to main menu\n";
	}


	*getHandler() << "\n";
}

void TelnetModeMainMenu::showAccount()
{
	showBanner();

	*getHandler() << " 1) Name:   " << account->getName() << "\n";
	*getHandler() << " 2) E-Mail: " << account->getEmail() << "\n";
	*getHandler() << " 3) Passphrase\n";
	*getHandler() << " 4) Return to main menu\n";

	*getHandler() << "\n";
}

void TelnetModeMainMenu::prompt()
{
	switch (state) {
	case STATE_MENU:
		*getHandler() << "Select:";
		break;
	case STATE_PLAY_SELECT:
		*getHandler() << "Character to play:";
		break;
	case STATE_DELETE_SELECT:
		*getHandler() << "Character to delete:";
		break;
	case STATE_DELETE_CONFIRM:
		*getHandler() << "Confirm:";
		break;
	case STATE_ACCOUNT:
		*getHandler() << "Select:";
		break;
	case STATE_CHANGE_NAME:
		*getHandler() << "Your real-life name:";
		break;
	case STATE_CHANGE_EMAIL:
		*getHandler() << "Your e-mail address:";
		break;
	case STATE_CHPASS_CHALLENGE:
		*getHandler() << "Your current passphrase:";
		break;
	case STATE_CHPASS_SELECT:
		*getHandler() << "New passphrase:";
		break;
	case STATE_CHPASS_CONFIRM:
		*getHandler() << "Confirm passphrase:";
		break;
	}
}

void TelnetModeMainMenu::showCreate()
{
	showBanner();

	*getHandler() << "Enter the name of your new character, or type " CBOLD "return" CNORMAL " to return to the main menu.\n\n";
}

void TelnetModeMainMenu::process(char* line)
{
	std::string input(line);

	switch (state) {
	case STATE_MENU:
		// erm, nothing?
		if (input.empty()) {
			showMain();
			// play?
		} else if (input == "1" || prefixMatch("play", input)) {
			state = STATE_PLAY_SELECT;
			showCharacters();
			// create?
		} else if (input == "2" || prefixMatch("create", input)) {
			// check max
			if (account->getCharList().size() >= account->getMaxCharacters()) {
				showMain();
				*getHandler() << "You already have the maximum number of characters allowed.\n\n";
			} else {
				// create
				ITelnetMode* mode = TelnetModeNewCharacter::create(getHandler(), account);
				if (mode != NULL)
					getHandler()->setMode(mode);
			}
			// account?
		} else if (input == "3" || prefixMatch("account", input)) {
			state = STATE_ACCOUNT;
			showAccount();
			// delete?
		} else if (input == "4" || prefixMatch("delete", input)) {
			state = STATE_DELETE_SELECT;
			showCharacters();
			// portal?
		} else if (input == "5" || prefixMatch("portal", input)) {
			*getHandler() << StreamMacro(MMessage.get("quit"));
			getHandler()->disconnect();
			// eh?
		} else {
			showMain();
			*getHandler() << "I don't understand.\n\n";
		}
		break;
	case STATE_PLAY_SELECT: {
		// conver input
		int inopt = tolong(input) - 1;
		if (inopt == (int)account->getCharList().size() || prefixMatch("return", input)) {
			state = STATE_MENU;
			showMain();
			break;
		}

		// whom did they ask for?
		int choice = -1;
		for (int i = 0; i < (int)account->getCharList().size(); ++i) {
			if (inopt == i || phraseMatch(account->getCharList()[i], input)) {
				choice = i;
				break;
			}
		}
		if (choice == -1) {
			state = STATE_MENU;
			showMain();
			*getHandler() << "Invalid character.\n\n";
			break;
		}

		// get player
		Player* player = MPlayer.load(account, account->getCharList()[choice]);
		if (player == NULL) {
			showCharacters();
			*getHandler() << CADMIN "Error: Character cannot be found." CNORMAL "\n\n";
			break;
		}

		// check active limit
		if (!player->isConnected()) {
			uint max_active = account->getMaxActive();
			if (account->getActive() >= max_active && !player->isActive()) {
				state = STATE_MENU;
				showMain();
				*getHandler() << "You already have the maximum number of characters active at a time.\n\n";
				break;
			}
		}

		// set play mode
		getHandler()->setMode(new TelnetModePlay(getHandler(), player));
		break;
	}
	case STATE_ACCOUNT:
		// change name?
		if (input == "1" || prefixMatch("name", input)) {
			state = STATE_CHANGE_NAME;
			showBanner();
			*getHandler() << "Current name: " << account->getName() << ".\n\n";
			*getHandler() << "You may correct your real-life name registered with this account.  To leave your name unchanged, do not enter and text and simple press return/enter.\n\n";
			// change email?
		} else if (input == "2" || prefixMatch("email", input)) {
			state = STATE_CHANGE_EMAIL;
			showBanner();
			*getHandler() << "Current email address: " << account->getEmail() << ".\n\n";
			*getHandler() << "You may correct the email address registered with this account.  To leave your email address unchanged, do not enter and text and simple press return/enter.\n\n";
			// change password?
		} else if (input == "3" || prefixMatch("passphrase", input)) {
			state = STATE_CHPASS_CHALLENGE;
			showBanner();
			*getHandler() << "You must enter your current passphrase before you may select a new one.\n\n";
			getHandler()->toggleEcho(false);
			// return?
		} else if (input.empty() || input == "4" || prefixMatch("return", input)) {
			showMain();
			state = STATE_MENU;
			// er?
		} else {
			showAccount();
			*getHandler() << "I don't understand.\n\n";
		}
		break;
	case STATE_CHANGE_NAME:
		// return to menu
		state = STATE_ACCOUNT;

		// not empty?  change
		if (!input.empty()) {
			account->setName(input);
			showAccount();
			*getHandler() << "Your name has been changed.\n\n";
		} else {
			showAccount();
		}
		break;
	case STATE_CHANGE_EMAIL:
		// return to menu
		state = STATE_ACCOUNT;

		// not empty?  change
		if (!input.empty()) {
			account->setEmail(input);
			showAccount();
			*getHandler() << "Your email address has been changed.\n\n";
		} else {
			showAccount();
		}
		break;
	case STATE_CHPASS_CHALLENGE:
		// check it
		if (!account->checkPassphrase(input)) {
			state = STATE_ACCOUNT;
			showAccount();
			*getHandler() << "Passphrase incorrect.\n\n";
			getHandler()->toggleEcho(true);
			break;
		}

		// alright, next state
		state = STATE_CHPASS_SELECT;
		showBanner();
		*getHandler() << "You may now enter a new passphrase.\n\n";
		break;
	case STATE_CHPASS_SELECT:
		// must be valid
		if (input.empty() || !MAccount.validPassphrase(input)) {
			state = STATE_ACCOUNT;
			showAccount();
			*getHandler() << "Passphrases must be at least " << ACCOUNT_PASS_MIN_LEN << " characters, and have both letters and numbers.  Passphrases may also contain symbols or punctuation characters.\n\n";
			getHandler()->toggleEcho(true);
			break;
		}

		// store it in temporary
		tmp = input;

		// next state
		state = STATE_CHPASS_CONFIRM;
		showBanner();
		*getHandler() << "You must now retype your passphrase to confirm that you have entered it correctly.\n\n";
		break;
	case STATE_CHPASS_CONFIRM:
		// re-enable echo
		getHandler()->toggleEcho(true);

		// confirm it
		if (input != tmp) {
			state = STATE_ACCOUNT;
			showAccount();
			*getHandler() << "Passphrases do not match.\n\n";
			break;
		}


		// set passphrase
		account->setPassphrase(input);

		// clear the tmp passphrase (no, this isn't for security - way too weak for that)
		tmp.clear();

		// all done
		state = STATE_ACCOUNT;
		showAccount();
		*getHandler() << "Your account passphrase has been changed.\n\n";
		break;
	case STATE_DELETE_SELECT: {
		// conver input
		int inopt = tolong(input) - 1;
		if (inopt == (int)account->getCharList().size() || prefixMatch("return", input)) {
			state = STATE_MENU;
			showMain();
			break;
		}

		// whom did they ask for?
		int choice = -1;
		for (int i = 0; i < (int)account->getCharList().size(); ++i) {
			if (inopt == i || phraseMatch(account->getCharList()[i], input)) {
				choice = i;
				break;
			}
		}
		if (choice == -1) {
			state = STATE_MENU;
			showMain();
			*getHandler() << "Invalid character.\n\n";
			break;
		}

		// store choice
		tmp = account->getCharList()[choice];

		// go to confirmation
		state = STATE_DELETE_CONFIRM;
		showBanner();
		*getHandler() << "Do you wish to delete " CPLAYER << account->getCharList()[choice] << CNORMAL "?\n\n";
		*getHandler() << CADMIN "Warning!!" CNORMAL "  If you delete a character, you will " CADMIN "not" CNORMAL " be able to get the character back, ever.\n\n";
		*getHandler() << "To confirm the deletion of this character, you must type " CBOLD "I am sure!" CNORMAL " with that exact spelling, punctuation, and capitalization.  If you do not wish to delete this character, simply press enter/return.\n\n";
		break;
	}
	case STATE_DELETE_CONFIRM:
		// return to menu
		state = STATE_MENU;
		showMain();

		// confirmed?  delete...
		if (input == "I am sure!") {
			// did delete work?
			if (MPlayer.destroy(tmp)) {
				*getHandler() << "Internal error; could not delete character.\n\n";
				tmp.clear();
				break;
			}

			// message
			account->delCharacter(tmp);
			Log::Info << "Account '" << account->getId() << "' deleted player '" << tmp << "'";
			*getHandler() << "Character " CPLAYER << tmp << CNORMAL " has been permanently deleted.\n\n";
			tmp.clear();
		} else {
			// no delete
			tmp.clear();
			*getHandler() << "Your character has not been deleted.\n\n";
		}
		break;
	}
}

// --- PLAY ---
int TelnetModePlay::initialize()
{
	player->connect(this);

	// start the player
	if (player->startSession() != 0) {
		*getHandler() << "\n" CADMIN "Failed to start your login session." CNORMAL "\n";
		Log::Warning << "Failed to start login session for '" << player->getId() << "'.";
		return -1;
	}

	// finish off
	Log::Info << player->getId() << " logged in";
	return 0;
}

void TelnetModePlay::pconnDisconnect()
{
	player = NULL;
	getHandler()->disconnect();
}

void TelnetModePlay::prompt()
{
	player->showPrompt();
}

void TelnetModePlay::process(char* line)
{
	player->processCommand(line);
}

void TelnetModePlay::finish()
{
	// disconnect if necessary
	getHandler()->setMode(new TelnetModeMainMenu(getHandler(), player->getAccount()));
}

void TelnetModePlay::shutdown()
{
	if (player && dynamic_cast<TelnetHandler*>(player->getConn()) == getHandler()) {
		player->disconnect();
		player = NULL;
	}
}
