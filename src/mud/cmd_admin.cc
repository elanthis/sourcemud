/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "awestr.h"
#include "server.h"
#include "command.h"
#include "player.h"
#include "streams.h"
#include "zone.h"
#include "account.h"

// handle changing of player password
class ChpassProcessor : public IProcessor
{
	public:
	ChpassProcessor (Player* s_player, Account* s_account) : IProcessor(s_player), account(s_account), pass() {}
	
	int init (void) { player->toggle_echo (false); return 0; }
	void finish (void) { player->toggle_echo (true); }
	int process (char*);
	const char* prompt (void);

	private:
	Account* account;
	String pass;
};

// shutdown the server
void command_admin_shutdown (Player* admin, char**) {
	*admin << CADMIN "Shutdown issued." CNORMAL "\n";
	Log::Admin << "Shutdown issued by " << admin->get_account()->get_id();
	ZoneManager.announce ("Shutting down, NOW!");
	AweMUD::shutdown();
}

// grant privileges to a user
void command_admin_grant (Player* admin, char** argv) {
	Account *account = AccountManager.get(argv[0]);
	if (account == NULL) {
		*admin << "Account '" << argv[0] << "' not found.\n";
		return;
	} else {
		AccessID access = AccessID::lookup(argv[1]);
		if (!account->has_access(access)) {
			account->grant_access(access);
			account->save();
			*admin << "Granted '" << AccessID::nameof(access) << "' to " << account->get_id() << ".\n";
			Log::Admin << admin->get_account()->get_id() << " granted '" << AccessID::nameof(access) << "' to " << account->get_id();
		} else {
			*admin << "Account " << account->get_id() << " already has '" << AccessID::nameof(access) << "'.\n";
		}
	}
}

// revoke privileges from a user
void command_admin_revoke (Player* admin, char** argv) {
	Account *account = AccountManager.get(argv[0]);
	if (account == NULL) {
		*admin << "Account '" << argv[0] << "' does not exist.\n";
		return;
	} else {
		AccessID access = AccessID::lookup(argv[1]);
		if (account->revoke_access(access)) {
			account->save();
			*admin << "Revoked '" << AccessID::nameof(access) << "' from " << account->get_id() << ".\n";
			Log::Admin << admin->get_account()->get_id() << " revoked '" << AccessID::nameof(access) << "' from " << account->get_id();
		} else {
			*admin << "Account " << account->get_id() << " doesn't have '" << AccessID::nameof(access) << "'.\n";
		}
	}
}

// block an IP address from connecting
void command_admin_blockip (Player* admin, char** argv) {
	*admin << CADMIN "TEMPORARLY DISABLED" CNORMAL << "\n";
	// FIXME: fix this
	/*
	if (server.tcp_players.add_deny(argv[0])) {
		*admin << "Adding '" << argv[0] << "' to block list.\n";
		Log::Info << admin->get_account()->get_id() << "' added '" << argv[0] << "' to the user block list.";
	} else {
		*admin << "Invalid deny value.\n";
	}
	*/
}

// change a user's password
void command_admin_chpass (Player* admin, char** argv) {
	// lookup user info
	Account* account;
	if ((account = AccountManager.get(argv[0])) == NULL) {
		*admin << CADMIN "Account '" << argv[0] << "' does not exist." CNORMAL "\n";
	} else {
		admin->add_processor(new ChpassProcessor(admin, account));
	}
}

// --- ChpassProcessor ---

const char *
ChpassProcessor::prompt (void)
{
	if (pass)
		return "Verify password:";
	else
		return "New password:";
}

int
ChpassProcessor::process (char *line)
{
	*player << "\n";
	if (pass) {
		if (!strcmp (pass, line)) {
			account->set_passphrase(line);
			*player << CADMIN "Password for account " CPLAYER << account->get_id() << CNORMAL " changed successfully." CNORMAL "\n";
			Log::Admin << "Password for account '" << account->get_id() << "' changed by '" << player->get_account()->get_id() << "'";
		} else {
			*player << CADMIN "Passwords do not match." CNORMAL "\n";
		}
		return true;
	} else {
		pass = line;
	}

	return 0;
}
