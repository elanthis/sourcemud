/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

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
void command_open(Creature* ch, String argv[]) {
	Portal* portal;
	if ((portal = ch->cl_find_portal (argv[0])) != NULL) {
		if (portal->is_door ())
			ch->do_open (portal);
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
void command_close(Creature* ch, String argv[]) {
	Portal* portal;
	if ((portal = ch->cl_find_portal (argv[0])) != NULL) {
		if (portal->is_door ())
			ch->do_close (portal);
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
void command_lock(Creature* ch, String argv[]) {
	Portal* portal;
	if ((portal = ch->cl_find_portal (argv[0])) != NULL) {
		if (portal->is_door ())
			ch->do_lock (portal);
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
void command_unlock(Creature* ch, String argv[]) {
	Portal* portal;
	if ((portal = ch->cl_find_portal (argv[0])) != NULL) {
		if (portal->is_door ())
			ch->do_unlock (portal);
		else
			*ch << StreamName(portal, DEFINITE, true) << " is not a door.\n";
	}
}
