/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "mud/creature.h"
#include "mud/command.h"
#include "mud/portal.h"

/* BEGIN COMMAND
 *
 * name: open
 * usage: open <portal>
 *
 * format: open :0*
 *
 * END COMMAND */
void command_open(Creature* ch, std::string argv[])
{
	Portal* portal;
	if ((portal = ch->clFindPortal(argv[0])) != NULL) {
		if (portal->isDoor())
			ch->doOpen(portal);
		else
			*ch << StreamName(portal, DEFINITE, true) << " is not a door.\n";
	}
}

/* BEGIN COMMAND
 *
 * name: close
 * usage: close <portal>
 *
 * format: close :0*
 *
 * END COMMAND */
void command_close(Creature* ch, std::string argv[])
{
	Portal* portal;
	if ((portal = ch->clFindPortal(argv[0])) != NULL) {
		if (portal->isDoor())
			ch->doClose(portal);
		else
			*ch << StreamName(portal, DEFINITE, true) << " is not a door.\n";
	}
}

/* BEGIN COMMAND
 *
 * name: lock
 * usage: lock <portal>
 *
 * format: lock :0*
 *
 * END COMMAND */
void command_lock(Creature* ch, std::string argv[])
{
	Portal* portal;
	if ((portal = ch->clFindPortal(argv[0])) != NULL) {
		if (portal->isDoor())
			ch->doLock(portal);
		else
			*ch << StreamName(portal, DEFINITE, true) << " is not a door.\n";
	}
}

/* BEGIN COMMAND
 *
 * name: unlock
 * usage: unlock <portal>
 *
 * format: unlock :0*
 *
 * END COMMAND */
void command_unlock(Creature* ch, std::string argv[])
{
	Portal* portal;
	if ((portal = ch->clFindPortal(argv[0])) != NULL) {
		if (portal->isDoor())
			ch->doUnlock(portal);
		else
			*ch << StreamName(portal, DEFINITE, true) << " is not a door.\n";
	}
}
