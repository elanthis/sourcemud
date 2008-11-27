/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "mud/creature.h"
#include "mud/command.h"
#include "mud/object.h"

/* BEGIN COMMAND
 *
 * name: drink
 * usage: drink <object>
 *
 * format: drink :0*
 *
 * END COMMAND */
void command_drink (Creature* ch, std::string argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_ANY);
	if (obj)
		ch->do_drink (obj);
}

/* BEGIN COMMAND
 *
 * name: eat
 * usage: eat <object>
 *
 * format: eat :0*
 *
 * END COMMAND */
void command_eat (Creature* ch, std::string argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_HELD);
	if (obj)
		ch->do_eat (obj);
}

/* BEGIN COMMAND
 *
 * name: kick
 * usage: kick <object>
 *
 * format: kick :0*
 *
 * END COMMAND */
void command_kick (Creature* ch, std::string argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_ANY, true);
	if (obj) {
		ch->do_kick (obj);
		return;
	}
	Portal* portal = ch->cl_find_portal (argv[0]);
	if (portal) {
		ch->do_kick (portal);
		return;
	}
}

/* BEGIN COMMAND
 *
 * name: raise
 * usage: raise <object>
 *
 * format: raise :0*
 *
 * END COMMAND */
void command_raise (Creature* ch, std::string argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_HELD);
	if (obj)
		ch->do_raise (obj);
}

/* BEGIN COMMAND
 *
 * name: read
 * usage: read <object>
 *
 * format: read :0*
 *
 * END COMMAND */
void command_read (Creature* ch, std::string argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_ANY);
	if (obj)
		ch->do_read (obj);
}

/* BEGIN COMMAND
 *
 * name: touch
 * usage: touch <object>
 *
 * format: touch :0*
 *
 * END COMMAND */
void command_touch (Creature* ch, std::string argv[])
{
	Object* obj = ch->cl_find_object (argv[0], GOC_ANY);
	if (obj)
		ch->do_touch (obj);
}
