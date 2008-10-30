/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "mud/creature.h"
#include "common/error.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/zone.h"
#include "mud/command.h"
#include "mud/player.h"
#include "mud/npc.h"
#include "mud/object.h"
#include "common/streams.h"
#include "mud/olc.h"
#include "mud/shadow-object.h"
#include "mud/unique-object.h"

using namespace OLC;

/* BEGIN COMMAND
 *
 * name: olc portals
 * usage: olc portals [<room>]
 *
 * format: olc portals :0%? (80)
 *
 * access: GM
 *
 * END COMMAND */

void command_olc_portals (Player *builder, String argv[])
{
	Room *room = NULL;
	
	if (argv[0].empty()) {
		room = ROOM(builder->get_room ());
		if (room == NULL) {
			*builder << "You are not in a room.\n";
			return;
		}
	} else {
		room = ZoneManager.get_room(argv[0]);
		if (room == NULL) {
			*builder << "Could not find room '" << argv[0] << "'.\n";
			return;
		}
	}

	*builder << "Portal list:\n";

	room->show_portals (*builder);
}
