/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "common/string.h"
#include "mud/server.h"
#include "mud/command.h"
#include "mud/player.h"
#include "common/streams.h"
#include "mud/zone.h"
#include "mud/account.h"

// shutdown the server
void command_admin_shutdown (Player* admin, String[]) {
	*admin << CADMIN "Shutdown issued." CNORMAL "\n";
	Log::Admin << "Shutdown issued by " << admin->get_account()->get_id();
	ZoneManager.announce (S("Shutting down, NOW!"));
	AweMUD::shutdown();
}

// grant privileges to a user
void command_admin_grant (Player* admin, String argv[]) {
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
void command_admin_revoke (Player* admin, String argv[]) {
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
void command_admin_blockip (Player* admin, String argv[]) {
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
