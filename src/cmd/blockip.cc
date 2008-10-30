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
 * name: admin blockip
 *
 * format: admin blockip :0% (80)
 *
 * access: ADMIN
 *
 * END COMMAND */

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
