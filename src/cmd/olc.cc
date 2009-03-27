/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/error.h"
#include "common/streams.h"
#include "common/string.h"
#include "mud/creature.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/zone.h"
#include "mud/command.h"
#include "mud/player.h"
#include "mud/npc.h"
#include "mud/object.h"
#include "mud/olc.h"

using namespace OLC;

/* BEGIN COMMAND
 *
 * name: olc create
 * usage: olc create [npc|object] [<template>]
 * usage: olc create room <name> [<zone>]
 * usage: olc create zone <name>
 * usage: olc create portal <dir> <target>
 *
 * format: olc create :0npc :1%? (80)
 * format: olc create :0object :1%? (80)
 * format: olc create :0room :1% :2%? (80)
 * format: olc create :0zone :1%? (80)
 * format: olc create :0portal :1% :2% (80)
 *
 * access: GM
 *
 * END COMMAND */
void command_olc_create(Player* builder, std::string argv[])
{
	// create npc from blueprint
	if (strEq(argv[0], "npc")) {
		Npc *new_npc = NULL;
		if (!argv[1].empty()) {
			new_npc = Npc::loadBlueprint(argv[1]);
			if (!new_npc) {
				*builder << "Failed to load blueprint '" << argv[1] << "'.\n";
				return;
			}
		} else {
			new_npc = new Npc();
		}

		new_npc->enter(builder->getRoom(), NULL);
		*builder << "New NPC " << StreamName(*new_npc, NONE) << " created.\n";
		// creat object from blueprint
	} else if (strEq(argv[0], "object")) {
		Object* new_object = NULL;
		if (!argv[1].empty()) {
			new_object = Object::loadBlueprint(argv[1]);
			if (!new_object) {
				*builder << "Failed to load blueprint '" << argv[1] << "'.\n";
				return;
			}
		} else {
			new_object = new Object();
		}

		builder->getRoom()->addObject(new_object);
		*builder << "New object " << StreamName(*new_object, NONE) << " created.\n";
		// create portal in room
	} else if (strEq(argv[0], "portal")) {
		Room* room = builder->getRoom();
		if (room == NULL) {
			*builder << "You are not in a room.\n";
			return;
		}

		PortalDir dir = PortalDir::lookup(argv[1]);
		if (!dir.valid()) {
			*builder << "Invalid portal direction.\n";
			return;
		}

		if (room->getPortalByDir(dir)) {
			*builder << "Portal for direction " << dir.getName() << " already exists.\n";
			return;
		}

		Room* target = MZone.getRoom(argv[2]);
		if (target == NULL) {
			*builder << "Target room '" << argv[2] << "' not found.\n";
			return;
		}

		Portal* portal = room->newPortal(dir);
		portal->setTarget(target->getId());
		*builder << "Portal created.\n";
		// create room in zone
	} else if (strEq(argv[0], "room")) {
		Zone *zone = NULL;
		if (!argv[2].empty()) {
			zone = MZone.getZone(argv[2]);
			if (zone == NULL) {
				*builder << "Zone '" << argv[2] << "' does not exist.\n";
				return;
			}
		} else {
			Room *our_room = ROOM(builder->getRoom());
			if (our_room)
				zone = our_room->getZone();
			if (zone == NULL) {
				*builder << "You are not in a zone.\n";
				return;
			}
		}

		if (MZone.getRoom(argv[1])) {
			*builder << "Room '" << argv[1] << "' already exists.\n";
			return;
		}

		Room *room = new Room();
		room->setId(argv[1]);
		room->setName(argv[1]);

		*builder << "Room '" << room->getId() << "' added.\n";
		zone->addRoom(room);
		// create zone
	} else if (strEq(argv[0], "zone")) {
		if (MZone.getZone(argv[1])) {
			*builder << "Zone '" << argv[1] << "' already exists.\n";
			return;
		}

		Zone *zone = new Zone();
		zone->setId(argv[1]);
		zone->setName(argv[1]);

		MZone.addZone(zone);
		*builder << "Zone '" << zone->getId() << "' added.\n";
	}
}

/* BEGIN COMMAND
 *
 * name: olc destroy
 * usage: olc destroy [<type>] <entity>
 *
 * format: olc destroy :1* (80)
 * format: olc destroy :0npc :1* (80)
 * format: olc destroy :0object :1* (80)
 * format: olc destroy :0room :1* (80)
 * format: olc destroy :0portal :1* (80)
 *
 * access: GM
 *
 * END COMMAND */
void command_olc_destroy(Player* builder, std::string argv[])
{
	Entity* entity;

	// valid form?
	if (!lookupEditable(builder, argv[0], argv[1], entity)) {
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
		if (ROOM(entity) == builder->getRoom()) {
			*builder << "You cannot delete the room you are in.\n";
			return;
		}

		entity->destroy();
		*builder << "Room '" << ROOM(entity)->getId() << "' destroyed.\n";
		return;
	}

	// find zone?
	/* FIXME
	if (ZONE(entity)) {
		if (builder->getRoom() && builder->getRoom()->getZone() == ZONE(entity)) {
			*builder << "You cannot delete the zone you are in.\n";
			return;
		}

		entity->destroy();
		*builder << "Zone '" << ZONE(entity)->get_id () << "' destroyed.\n";
		return;
	}
	*/
}

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
void command_olc_portals(Player *builder, std::string argv[])
{
	Room *room = NULL;

	if (argv[0].empty()) {
		room = ROOM(builder->getRoom());
		if (room == NULL) {
			*builder << "You are not in a room.\n";
			return;
		}
	} else {
		room = MZone.getRoom(argv[0]);
		if (room == NULL) {
			*builder << "Could not find room '" << argv[0] << "'.\n";
			return;
		}
	}

	*builder << "Portal list:\n";

	room->showPortals(*builder);
}
