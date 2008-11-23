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
 * name: kneel
 *
 * format: kneel
 *
 * END COMMAND */
void command_kneel(Creature* ch, String[]) {
	ch->do_position (CreaturePosition::KNEEL);
}

/* BEGIN COMMAND
 *
 * name: lay
 *
 * format: lay
 *
 * END COMMAND */
void command_lay(Creature* ch, String[]) {
	ch->do_position (CreaturePosition::LAY);
}

/* BEGIN COMMAND
 *
 * name: sit
 *
 * format: sit
 *
 * END COMMAND */
void command_sit(Creature* ch, String[]) {
	ch->do_position (CreaturePosition::SIT);
}

/* BEGIN COMMAND
 *
 * name: stand
 *
 * format: stand
 *
 * END COMMAND */
void command_stand(Creature* ch, String[]) {
	ch->do_position (CreaturePosition::STAND);
}
