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
 * name: stand
 *
 * format: stand
 *
 * END COMMAND */

void command_stand (Creature* ch, String[]) {
	ch->do_position (CreaturePosition::STAND);
}
