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
 * name: sit
 *
 * format: sit
 *
 * END COMMAND */

void command_sit (Creature* ch, String[]) {
	ch->do_position (CreaturePosition::SIT);
}
