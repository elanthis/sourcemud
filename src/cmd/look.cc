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
 * name: look
 * usage: look
 * usage: look [at|in|on|under] <entity>
 *
 * format: look (20)
 * format: look :1* (20)
 * format: look :0at :1* (20)
 * format: look :0in :1* (20)
 * format: look :0on :1* (20)
 * format: look :0under :1* (20)
 *
 * END COMMAND */

void command_look(Creature* ch, std::string argv[])
{
	// have we a target (argv[1])
	if (argv[1].empty()) {
		ch->doLook();
		return;
	}

	// looking in/on/etc. container?
	if (!argv[0].empty() && argv[0] != "at") {
		Object* obj = ch->clFindObject(argv[1], GOC_ANY);
		if (obj) {
			if (argv[0] == "on")
				ch->doLook(obj, ObjectLocation::ON);
			else if (argv[0] == "in")
				ch->doLook(obj, ObjectLocation::IN);
			else
				ch->doLook(obj, ObjectLocation::NONE);
		}
		return;
	}

	// generic find
	Entity* entity = ch->clFindAny(argv[1]);
	if (entity != NULL) {
		// creature?
		if (CHARACTER(entity))
			ch->doLook((Creature*)(entity));
		// object?
		else if (OBJECT(entity))
			ch->doLook((Object*)(entity), ObjectLocation::NONE);
		// eixt?
		else if (PORTAL(entity))
			ch->doLook((Portal*)(entity));
	}
}
