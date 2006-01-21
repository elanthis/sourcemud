/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/char.h"
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

void command_create (Player* builder, char** argv)
{
	// create npc from blueprint
	if (str_eq (argv[0], "npc")) {
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
	} else if (str_eq (argv[0], "object")) {
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
	// create exit in room
	} else if (str_eq(argv[0], "exit")) {
		Room* room = builder->get_room();
		if (room == NULL) {
			*builder << "You are not in a room.\n";
			return;
		}

		// crate exit
		RoomExit *exit = room->new_exit ();

		if (exit) {
			// have a name to set?
			if (argv[1] != NULL) {
				// try setting a direction as well
				ExitDir dir = ExitDir::lookup(argv[1]);
				if (dir.valid())
					exit->set_dir (dir);

				// set name
				exit->set_name (argv[1]);

				// set target
				if (argv[2] != NULL) {
					Room* target = ZoneManager.get_room(argv[2]);
					if (target != NULL) {
						// do target set
						exit->set_target(argv[2]);

						// make a reciprical exit
						if (dir.valid()) {
							ExitDir op = dir.get_opposite();
							if (!target->get_exit_by_dir(op)) {
								RoomExit* op_exit = target->new_exit();
								op_exit->set_dir(op);
								op_exit->set_name(op.get_name());
								op_exit->set_target(room->get_id());
								*builder << "Reciprical exit created.\n";
							} else {
								*builder << "Reciprical exits.\n";
							}
						}
					} else {
						*builder << "Could not find room '" << argv[2] << "'.\n";
					}
				}
			}

			*builder << "Exit created.\n";
		} else {
			// failed error
			*builder << "Failed to create new exit.\n";
		}
	// create room in zone
	} else if (str_eq(argv[0], "room")) {
		Zone *zone = NULL;
		if (argv[2] != NULL) {
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
	} else if (str_eq(argv[0], "zone")) {
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

void command_edit (Player* builder, char** argv)
{
	Entity* entity;
	
	// get choice
	if (!lookup_editable(builder, argv[0], entity)) {
		return;
	}
	if (entity == NULL)
		return;

	// add processor
	builder->add_processor(new EditProcessor(builder, entity));
}

void command_destroy (Player* builder, char** argv)
{
	Entity* entity;

	// valid form?
	if (!lookup_editable(builder, argv[0], entity)) {
		return;
	}

	// players not allowed
	if (PLAYER(entity)) {
		*builder << "You cannot destroy players.\n";
		return;
	}

	// find character?
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

	// find exit?
	if (ROOMEXIT(entity)) {
		entity->destroy();
		*builder << "Exit " << StreamName(ROOMEXIT(entity)) << " destroyed.\n";
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

void command_exitlist (Player *builder, char** argv)
{
	Room *room = NULL;
	
	if (argv[0] == NULL) {
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

	*builder << "Exit list:\n";

	room->show_exits (*builder);
}

void command_bvision (Player* builder, char** argv)
{
	// no arg
	if (argv[0] == NULL) {
		*builder << "Builder vision is currently " CADMIN << (builder->has_bvision() ? "on" : "off") << CNORMAL ".\n";
	// arg is on
	} else if (str_eq(argv[0], "on")) {
		builder->set_bvision(true);
		*builder << "Builder vision turned on.\n";
	// arg is off
	} else if (str_eq(argv[0], "off")) {
		builder->set_bvision(false);
		*builder << "Builder vision turned off.\n";
	// eh?
	} else {
		*builder << "You must specify 'on' or 'off'.\n";
	}
}
