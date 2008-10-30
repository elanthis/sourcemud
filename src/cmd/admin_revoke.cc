/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common/string.h"
#include "mud/server.h"
#include "mud/command.h"
#include "mud/player.h"
#include "common/streams.h"
#include "mud/zone.h"
#include "mud/account.h"

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
