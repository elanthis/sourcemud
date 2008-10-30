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
 * name: lay
 *
 * format: lay
 *
 * END COMMAND */

void command_lay (Creature* ch, String[]) {
	ch->do_position (CreaturePosition::LAY);
}
