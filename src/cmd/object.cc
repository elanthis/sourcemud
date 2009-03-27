/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
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
void command_drink(Creature* ch, std::string argv[])
{
	Object* obj = ch->clFindObject(argv[0], GOC_ANY);
	if (obj)
		ch->doDrink(obj);
}

/* BEGIN COMMAND
 *
 * name: eat
 * usage: eat <object>
 *
 * format: eat :0*
 *
 * END COMMAND */
void command_eat(Creature* ch, std::string argv[])
{
	Object* obj = ch->clFindObject(argv[0], GOC_HELD);
	if (obj)
		ch->doEat(obj);
}

/* BEGIN COMMAND
 *
 * name: kick
 * usage: kick <object>
 *
 * format: kick :0*
 *
 * END COMMAND */
void command_kick(Creature* ch, std::string argv[])
{
	Object* obj = ch->clFindObject(argv[0], GOC_ANY, true);
	if (obj) {
		ch->doKick(obj);
		return;
	}
	Portal* portal = ch->clFindPortal(argv[0]);
	if (portal) {
		ch->doKick(portal);
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
void command_raise(Creature* ch, std::string argv[])
{
	Object* obj = ch->clFindObject(argv[0], GOC_HELD);
	if (obj)
		ch->doRaise(obj);
}

/* BEGIN COMMAND
 *
 * name: read
 * usage: read <object>
 *
 * format: read :0*
 *
 * END COMMAND */
void command_read(Creature* ch, std::string argv[])
{
	Object* obj = ch->clFindObject(argv[0], GOC_ANY);
	if (obj)
		ch->doRead(obj);
}

/* BEGIN COMMAND
 *
 * name: touch
 * usage: touch <object>
 *
 * format: touch :0*
 *
 * END COMMAND */
void command_touch(Creature* ch, std::string argv[])
{
	Object* obj = ch->clFindObject(argv[0], GOC_ANY);
	if (obj)
		ch->doTouch(obj);
}
