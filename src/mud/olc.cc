/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/char.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/command.h"
#include "mud/player.h"
#include "mud/npc.h"
#include "mud/zone.h"
#include "mud/object.h"
#include "common/streams.h"
#include "mud/help.h"
#include "mud/olc.h"
#include "mud/telnet.h"
#include "mud/account.h"
#include "mud/zmp.h"

enum OLCMode {
	OLC_MODE_SET,
	OLC_MODE_GET,
	OLC_MODE_APPEND,
	OLC_MODE_LIST
};

#define OLC_BEGIN_TYPE(type) \
	if (1) { \
		type* edit = dynamic_cast<type*>(olc_entity); \
	if (edit != NULL) {
#define OLC_BEGIN_ATTR(attr_name) \
	if (olc_mode == OLC_MODE_LIST || phrase_match(#attr_name, olc_attr)) { \
		const char* attr = #attr_name; \
		olc_ok = true; \
		if (0) {
#define OLC_SET \
	} else if (olc_mode == OLC_MODE_SET) {
#define OLC_GET \
	} else if (olc_mode == OLC_MODE_GET || olc_mode == OLC_MODE_LIST) {
#define OLC_APPEND \
	} else if (olc_mode == OLC_MODE_APPEND) {
#define OLC_DISPLAY(expr) \
		*user << CSPECIAL << attr << CNORMAL << ": " << (expr) << "\n";
#define OLC_END_ATTR \
	} else if (olc_mode == OLC_MODE_SET) { \
		*user << "The attribute " << attr << " is read-only.\n"; \
		return; \
	} else if (olc_mode == OLC_MODE_GET) { \
		*user << "The attribute " << attr << " is write-only.\n"; \
		return; \
	} else if (olc_mode == OLC_MODE_APPEND) { \
		*user << "The attribute " << attr << " cannot be appended to.\n"; \
		return; \
	} }
#define OLC_END_TYPE \
	} }

// private stuff
namespace OLC {
	// lookup entity
	bool 
	lookup_editable (Player* builder, char* name, Entity*& entity)
	{
		// init
		entity = NULL;

		enum { tNone, tPlayer, tChar, tNPC, tObject, tRoom, tExit, tZone } type = tNone;

		if (!commands::is_arg(name))
			return false;

		// is name comprised of 'room' or 'zone' or whatever?
		char* tname= commands::get_arg(&name);
		if (str_eq(tname, "room"))
			type = tRoom;
		else if (str_eq(tname, "zone"))
			type = tZone;
		else if (str_eq(tname, "player"))
			type = tPlayer;
		else if (str_eq(tname, "npc"))
			type = tNPC;
		else if (str_eq(tname, "exit"))
			type = tExit;
		else if (str_eq(tname, "object"))
			type = tObject;
		else
			commands::fix_arg(tname, &name);

		if (type != tRoom && type != tZone) {
			if (!commands::is_arg(name))
				return false;
		}

		// find character?
		if (type == tNone || type == tChar) {
			Character* character = builder->cl_find_character (name, true);
			if (character != NULL) {
				entity = character;
				return true;
			}
		}
		// limit to NPCs?
		if (type == tNPC) {
			Character* character = builder->cl_find_character (name);
			if (NPC(character)) {
				entity = character;
				return true;
			} else if (character != NULL) {
				*builder << "Character " << StreamName(character) << " is not an NPC.\n";
				return false;
			}
		}
		// limit to Players?
		if (type == tPlayer) {
			Player* player = PlayerManager.get(name);
			if (player != NULL) {
				entity = player;
				return true;
			} else {
				*builder << "Cannot find player '" << name << "'.\n";
				return false;
			}
		}

		// find object?
		if (type == tNone || type == tObject) {
			Object* object = builder->cl_find_object (name, GOC_ANY, true);
			if (object != NULL) {
				entity = object;
				return true;
			}
		}

		// find exit?
		if (type == tNone) {
			RoomExit* exit = builder->cl_find_exit (name, true);
			if (exit != NULL) {
				entity = exit;
				return true;
			}
		}
		if (type == tExit) {
			Room* room = builder->get_room();
			if (!room) {
				*builder << "You are not in a room.\n";
				return false;
			}
			RoomExit* exit = builder->cl_find_exit (name, true);
			if (exit == NULL) {
				*builder << "Cannot find exit '" << name << "'.\n";
				return false;
			}
			entity = exit;
			return true;
		}

		// find room?
		if (type == tRoom) {
			Room* room = NULL;
			if (commands::is_arg(name)) 
				room = ZoneManager.get_room(name);
			else
				room = builder->get_room();
			if (room != NULL) {
				entity = room;
				return true;
			} else {
				*builder << "Cannot find room '" << name << "'.\n";
				return false;
			}
		}

		// find zone?
		if (type == tZone) {
			Zone* zone = NULL;
			if (commands::is_arg(name))
				zone = ZoneManager.get_zone(name);
			else
				zone = builder->get_room() ? builder->get_room()->get_zone() : NULL;
			if (zone != NULL) {
				entity = zone;
				return true;
			} else {
				*builder << "Cannot find zone '" << name << "'.\n";
				return false;
			}
		}

		// none found
		*builder << "Cannot find '" << name << "'.\n";
		return false;
	}

	void
	do_olc (Player* user, OLCMode olc_mode, Entity* olc_entity, StringArg olc_attr, StringArg value)
	{
		bool olc_ok = false;

		// ----- BEGIN OLC BLOCK -----

OLC_BEGIN_TYPE(Entity)
	OLC_BEGIN_ATTR(uniqid)
		OLC_GET
			OLC_DISPLAY(UniqueIDManager.encode(edit->get_uid()))
	OLC_END_ATTR
	OLC_BEGIN_ATTR(name)
		OLC_GET
			OLC_DISPLAY(edit->get_name().get_name())
	OLC_END_ATTR
	OLC_BEGIN_ATTR(desc)
		OLC_GET
			OLC_DISPLAY(edit->get_desc())
	OLC_END_ATTR
OLC_END_TYPE

OLC_BEGIN_TYPE(Character)
	OLC_BEGIN_ATTR(hp)
		OLC_GET
			OLC_DISPLAY(edit->get_hp())
		OLC_SET
			edit->set_hp(tolong(value));
	OLC_END_ATTR
OLC_END_TYPE

OLC_BEGIN_TYPE(Object)
	OLC_BEGIN_ATTR(blueprint)
		OLC_GET
			OLC_DISPLAY(edit->get_blueprint()->get_id())
		OLC_SET
			ObjectBlueprint* blueprint = ObjectBlueprintManager.lookup(value);
			if (blueprint != NULL)
				edit->set_blueprint(blueprint);
			else
				*user << "Blueprint not found.\n";
	OLC_END_ATTR
OLC_END_TYPE

OLC_BEGIN_TYPE(Npc)
	OLC_BEGIN_ATTR(blueprint)
		OLC_GET
			OLC_DISPLAY(edit->get_blueprint()->get_id())
		OLC_SET
			NpcBlueprint* blueprint = NpcBlueprintManager.lookup(value);
			if (blueprint != NULL)
				edit->set_blueprint(blueprint);
			else
				*user << "Blueprint not found.\n";
	OLC_END_ATTR
OLC_END_TYPE

OLC_BEGIN_TYPE(RoomExit)
OLC_END_TYPE
		
		// ----- END OLC BLOCK -----

		if (!olc_ok && olc_mode != OLC_MODE_LIST)
			*user << "The attribute '" << olc_attr << "' does not exist.\n";
	}
}
