/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/player.h"
#include "mud/server.h"
#include "mud/macro.h"
#include "mud/command.h"
#include "common/streams.h"
#include "common/mail.h"
#include "mud/color.h"
#include "mud/telnet.h"
#include "mud/settings.h"

#include "config.h"

/* BEGIN COMMAND
 *
 * name: server
 *
 * END COMMAND */

void command_server (Player *player, String[])
{
	*player << "AweMUD V" PACKAGE_VERSION "\nBuild: " __DATE__ " " __TIME__ "\nUptime: " << AweMUD::get_uptime() << "\n";
}
