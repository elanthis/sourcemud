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
 * name: olc destroy
 * usage: olc destroy [<type>] <entity>
 *
 * format: olc destroy:0(npc,object,room,portal)? :1* (80)
 *
 * access: GM
 *
 * END COMMAND */

void command_olc_destroy (Player* builder, String argv[])
{
	Entity* entity;

	// valid form?
	if (!lookup_editable(builder, argv[0], argv[1], entity)) {
		return;
	}

	// players not allowed
	if (PLAYER(entity)) {
		*builder << "You cannot destroy players.\n";
		return;
	}

	// find creature?
	if (NPC(entity)) {
		entity->destroy();
		*builder << "NPC " << StreamName(NPC(entity)) << " destroyed.\n";
		return;
	}

	// find object?
	if (OBJECT(entity)) {
		entity->destroy();
		*builder << "Object " << StreamName(OBJECT(entity)) << " destroyed.\n";
		return;
	}

	// find portal?
	if (PORTAL(entity)) {
		entity->destroy();
		*builder << "Portal " << StreamName(PORTAL(entity)) << " destroyed.\n";
		return;
	}

	// find room?
	if (ROOM(entity)) {
		if (ROOM(entity) == builder->get_room ()) {
			*builder << "You cannot delete the room you are in.\n";
			return;
		}

		entity->destroy();
		*builder << "Room '" << ROOM(entity)->get_id () << "' destroyed.\n";
		return;
	}

	// find zone?
	/* FIXME
	if (ZONE(entity)) {
		if (builder->get_room() && builder->get_room()->get_zone() == ZONE(entity)) {
			*builder << "You cannot delete the zone you are in.\n";
			return;
		}

		entity->destroy();
		*builder << "Zone '" << ZONE(entity)->get_id () << "' destroyed.\n";
		return;
	}
	*/
}
