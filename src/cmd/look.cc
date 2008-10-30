/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/creature.h"
#include "common/error.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/command.h"
#include "mud/body.h"
#include "mud/player.h"
#include "mud/macro.h"
#include "common/streams.h"
#include "mud/object.h"

/* BEGIN COMMAND
 *
 * name: look
 * usage: look
 * usage: look [at|in|on|under] <entity>
 *
 * format: look (20)
 * format: look :0(at,in,on,under)? :1* (20)
 *
 * END COMMAND */

void command_look (Creature* ch, String argv[]) {
	// have we a target (argv[1])
	if (!argv[1]) {
		ch->do_look ();
		return;
	}

	// looking in/on/etc. container?
	if (argv[0] && argv[0] != "at") {
		Object* obj = ch->cl_find_object (argv[1], GOC_ANY);
		if (obj) {
			if (argv[0] == "on")
				ch->do_look (obj, ObjectLocation::ON);
			else if (argv[0] == "in")
				ch->do_look (obj, ObjectLocation::IN);
			else
				ch->do_look (obj, ObjectLocation::NONE);
		}
		return;
	}

	// generic find
	Entity* entity = ch->cl_find_any(argv[1]);
	if (entity != NULL) {
		// creature?
		if (CHARACTER(entity))
			ch->do_look((Creature*)(entity));
		// object?
		else if (OBJECT(entity))
			ch->do_look((Object*)(entity), ObjectLocation::NONE);
		// eixt?
		else if (PORTAL(entity))
			ch->do_look((Portal*)(entity));
	}
}
