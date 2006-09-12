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
#include "mud/zone.h"
#include "mud/command.h"
#include "mud/player.h"
#include "mud/npc.h"
#include "mud/object.h"
#include "common/streams.h"
#include "mud/olc.h"

using namespace OLC;

void command_create (Player* builder, String argv[])
{
	// create npc from blueprint
	if (str_eq (argv[0], S("npc"))) {
		Npc *new_npc = NULL;
		if (argv[1]) {
			new_npc = Npc::load_blueprint(argv[1]);
			if (!new_npc) {
				*builder << "Failed to load blueprint '" << argv[1] << "'.\n";
				return;
			}
		} else {
			new_npc = new Npc();
		}

		new_npc->enter (builder->get_room(), NULL);
		*builder << "New NPC " << StreamName(*new_npc, NONE) << " created.\n";
	// creat object from blueprint
	} else if (str_eq (argv[0], S("object"))) {
		Object* new_object = NULL;
		if (argv[1]) {
			new_object = Object::load_blueprint(argv[1]);
			if (!new_object) {
				*builder << "Failed to load blueprint '" << argv[1] << "'.\n";
				return;
			}
		} else {
			new_object = new Object();
		}

		builder->get_room()->add_object (new_object);
		*builder << "New object " << StreamName(*new_object, NONE) << " created.\n";
	// create portal in room
	} else if (str_eq(argv[0], S("portal"))) {
		Room* room = builder->get_room();
		if (room == NULL) {
			*builder << "You are not in a room.\n";
			return;
		}

		PortalDir dir = PortalDir::lookup(argv[1]);
		if (!dir.valid()) {
			*builder << "Invalid portal direction.\n";
			return;
		}

		if (room->get_portal_by_dir(dir)) {
			*builder << "Portal for direction " << dir.get_name() << " already exists.\n";
			return;
		}

		Room* target = ZoneManager.get_room(argv[2]);
		if (target == NULL) {
			*builder << "Target room '" << argv[2] << "' not found.\n";
			return;
		}

		Portal* portal = room->new_portal(dir);
		portal->set_target(target->get_id());
		*builder << "Portal created.\n";
	// create room in zone
	} else if (str_eq(argv[0], S("room"))) {
		Zone *zone = NULL;
		if (!argv[2].empty()) {
			zone = ZoneManager.get_zone(argv[2]);
			if (zone == NULL) {
				*builder << "Zone '" << argv[2] << "' does not exist.\n";
				return;
			}
		} else {
			Room *our_room = ROOM(builder->get_room());
			if (our_room)
				zone = our_room->get_zone();
			if (zone == NULL) {
				*builder << "You are not in a zone.\n";
				return;
			}
		}

		if (ZoneManager.get_room (argv[1])) {
			*builder << "Room '" << argv[1] << "' already exists.\n";
			return;
		}

		Room *room = new Room ();
		room->set_id (argv[1]);
		room->set_name (argv[1]);

		*builder << "Room '" << room->get_id () << "' added.\n";
		zone->add_room (room);
	// create zone
	} else if (str_eq(argv[0], S("zone"))) {
		if (ZoneManager.get_zone (argv[1])) {
			*builder << "Zone '" << argv[1] << "' already exists.\n";
			return;
		}

		Zone *zone = new Zone ();
		zone->set_id (argv[1]);
		zone->set_name (argv[1]);

		ZoneManager.add_zone (zone);
		*builder << "Zone '" << zone->get_id () << "' added.\n";
	}
}

void command_destroy (Player* builder, String argv[])
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
	if (ZONE(entity)) {
		if (builder->get_room() && builder->get_room()->get_zone() == ZONE(entity)) {
			*builder << "You cannot delete the zone you are in.\n";
			return;
		}

		entity->destroy();
		*builder << "Zone '" << ZONE(entity)->get_id () << "' destroyed.\n";
		return;
	}
}

void command_portallist (Player *builder, String argv[])
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
