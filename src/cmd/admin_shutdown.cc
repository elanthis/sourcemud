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

/* BEGIN COMMAND
 *
 * name: admin shutdown
 *
 * format: admin shutdown (80)
 *
 * access: ADMIN
 *
 * END COMMAND */

// shutdown the server
void command_admin_shutdown (Player* admin, String[]) {
	*admin << CADMIN "Shutdown issued." CNORMAL "\n";
	Log::Admin << "Shutdown issued by " << admin->get_account()->get_id();
	ZoneManager.announce (S("Shutting down, NOW!"));
	AweMUD::shutdown();
}
