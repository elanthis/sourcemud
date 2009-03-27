/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/error.h"
#include "common/streams.h"
#include "mud/creature.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/command.h"
#include "mud/body.h"
#include "mud/player.h"
#include "mud/macro.h"
#include "mud/object.h"

/* BEGIN COMMAND
 *
 * name: emote
 * usage: emote <action>
 *
 * format: emote :0*
 *
 * END COMMAND */
void command_emote(Creature* ch, std::string argv[])
{
	ch->doEmote(argv[0]);
}

/* BEGIN COMMAND
 *
 * name: laugh
 * usage: laugh [evilly]
 * usage: laugh [evilly] [at] <target>
 *
 * format: laugh :1evilly?
 * format: laugh :1evilly? at? :0*
 *
 * END COMMAND */
void command_laugh(Creature* ch, std::string argv[])
{
	// lookup target, if any
	Creature* target = NULL;
	if (!argv[0].empty())
		if ((target = ch->clFindCreature(argv[0])) == NULL)
			return;

	// determine adverb
	if (argv[1] == "evilly") {
		if (target) {
			*ch << "You laugh evilly at " << StreamName(target, DEFINITE) << ".\n";
			*target << StreamName(ch, INDEFINITE, true) << " laughs evilly at you.\n";
			*ch->getRoom() << StreamIgnore(ch) << StreamIgnore(target) << StreamName(ch, INDEFINITE, true) << " laughs evilly at " << StreamName(target, INDEFINITE) << ".\n";
		} else {
			*ch << "You laugh evilly.\n";
			*ch->getRoom() << StreamIgnore(ch) << StreamName(ch, INDEFINITE, true) << " laughs evilly.";
		}
		// no adverb
	} else {
		if (target) {
			*ch << "You laugh at " << StreamName(target, DEFINITE) << ".\n";
			*target << StreamName(ch, INDEFINITE, true) << " laughs at you.\n";
			*ch->getRoom() << StreamIgnore(ch) << StreamIgnore(target) << StreamName(ch, INDEFINITE, true) << " laughs at " << StreamName(target, INDEFINITE) << ".\n";
		} else {
			*ch << "You laugh.\n";
			*ch->getRoom() << StreamIgnore(ch) << StreamName(ch, INDEFINITE, true) << " laughs.";
		}
	}
}
