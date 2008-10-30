/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/creature.h"
#include "mud/room.h"
#include "mud/command.h"
#include "mud/object.h"

/* BEGIN COMMAND
 *
 * name: put
 * usage: put <item> in|on|under <container>
 *
 * format: put :0* :2(in,on,under) :1*
 *
 * END COMMAND */

void command_put (Creature* ch, String argv[]) {
	Object* obj = ch->cl_find_object (argv[0], GOC_HELD);
	if (!obj) 
		return;

	Object* cobj = ch->cl_find_object (argv[2], GOC_ANY);
	if (!cobj)
		return;

	if (argv[1] == "on")
		ch->do_put (obj, cobj, ObjectLocation::ON);
	else if (argv[1] == "in")
		ch->do_put (obj, cobj, ObjectLocation::IN);
}
