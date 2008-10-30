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
 * name: admin grant
 * usage: admin grant <player> <privilege>
 *
 * format: admin grant :0% :1% (80)
 *
 * access: ADMIN
 *
 * END COMMAND */

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
