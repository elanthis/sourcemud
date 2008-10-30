/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
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

/* BEGIN COMMAND
 *
 * name: man
 * usage: man <command>
 *
 * format: man :0*?
 *
 * END COMMAND */

void command_man (Player* player, String argv[])
{
	StreamControl stream(player);
	CommandManager.show_man(stream, argv[0]);
}
