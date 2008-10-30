/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
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
