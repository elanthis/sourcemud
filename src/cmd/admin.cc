/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/string.h"
#include "common/streams.h"
#include "mud/server.h"
#include "mud/command.h"
#include "mud/player.h"
#include "mud/zone.h"
#include "mud/account.h"

/* BEGIN COMMAND
 *
 * name: admin grant
 * usage: admin grant <player> <privilege>
 *
 * format: admin grant :0% :1% (80)
 *
 * access: ADMIN
 *
 * END COMMAND */
void command_admin_grant (Player* admin, std::string argv[]) {
	std::tr1::shared_ptr<Account> account = MAccount.get(argv[0]);
	if (account == NULL) {
		*admin << "Account '" << argv[0] << "' not found.\n";
		return;
	} else {
		AccessID access = AccessID::lookup(argv[1]);
		if (!account->hasAccess(access)) {
			account->grantAccess(access);
			account->save();
			*admin << "Granted '" << AccessID::nameof(access) << "' to " << account->getId() << ".\n";
			Log::Admin << admin->get_account()->getId() << " granted '" << AccessID::nameof(access) << "' to " << account->getId();
		} else {
			*admin << "Account " << account->getId() << " already has '" << AccessID::nameof(access) << "'.\n";
		}
	}
}

/* BEGIN COMMAND
 *
 * name: admin revoke
 * usage: admin revoke <player> <privilege>
 *
 * format: admin revoke :0% :1% (80)
 *
 * access: ADMIN
 *
 * END COMMAND */
void command_admin_revoke (Player* admin, std::string argv[]) {
	std::tr1::shared_ptr<Account> account = MAccount.get(argv[0]);
	if (account == NULL) {
		*admin << "Account '" << argv[0] << "' does not exist.\n";
		return;
	} else {
		AccessID access = AccessID::lookup(argv[1]);
		if (account->revokeAccess(access)) {
			account->save();
			*admin << "Revoked '" << AccessID::nameof(access) << "' from " << account->getId() << ".\n";
			Log::Admin << admin->get_account()->getId() << " revoked '" << AccessID::nameof(access) << "' from " << account->getId();
		} else {
			*admin << "Account " << account->getId() << " doesn't have '" << AccessID::nameof(access) << "'.\n";
		}
	}
}

/* BEGIN COMMAND
 *
 * name: admin shutdown
 *
 * format: admin shutdown (80)
 *
 * access: ADMIN
 *
 * END COMMAND */
void command_admin_shutdown (Player* admin, std::string[]) {
	*admin << CADMIN "Shutdown issued." CNORMAL "\n";
	Log::Admin << "Shutdown issued by " << admin->get_account()->getId();
	MZone.announce ("Shutting down, NOW!");
	MUD::shutdown();
}

/* BEGIN COMMAND
 *
 * name: admin blockip
 *
 * format: admin blockip :0% (80)
 *
 * access: ADMIN
 *
 * END COMMAND */
void command_admin_blockip (Player* admin, std::string argv[]) {
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
