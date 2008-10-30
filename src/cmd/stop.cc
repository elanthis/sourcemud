/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/creature.h"
#include "mud/command.h"

/* BEGIN COMMAND
 *
 * name: stop
 *
 * format: stop
 *
 * END COMMAND */

void command_stop (Creature* ch, String argv[])
{
	ch->cancel_action();
}
