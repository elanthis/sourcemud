/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/streams.h"
#include "common/string.h"
#include "mud/creature.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/command.h"
#include "mud/player.h"
#include "mud/npc.h"
#include "mud/zone.h"
#include "mud/object.h"
#include "mud/help.h"
#include "mud/olc.h"
#include "mud/account.h"
#include "net/telnet.h"

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
	if (olc_mode == OLC_MODE_LIST || prefixMatch(#attr_name, olc_attr)) { \
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
namespace OLC
{
	// lookup entity
	bool
	lookupEditable(Player* builder, const std::string& tname, const std::string& name, Entity*& entity)
	{
		// init
		entity = NULL;
		enum { tNone, tPlayer, tCreature, tNPC, tObject, tRoom, tPortal, tZone } type = tNone;

		// is name comprised of 'room' or 'zone' or whatever?
		if (strEq(tname, "room"))
			type = tRoom;
		else if (strEq(tname, "zone"))
			type = tZone;
		else if (strEq(tname, "player"))
			type = tPlayer;
		else if (strEq(tname, "npc"))
			type = tNPC;
		else if (strEq(tname, "portal"))
			type = tPortal;
		else if (strEq(tname, "object"))
			type = tObject;

		if (type != tRoom && type != tZone) {
			if (name.empty())
				return false;
		}

		// find creature?
		if (type == tNone || type == tCreature) {
			Creature* creature = builder->clFindCreature(name, true);
			if (creature != NULL) {
				entity = creature;
				return true;
			}
		}
		// limit to NPCs?
		if (type == tNPC) {
			Creature* creature = builder->clFindCreature(name);
			if (NPC(creature)) {
				entity = creature;
				return true;
			} else if (creature != NULL) {
				*builder << "Creature " << StreamName(creature) << " is not an NPC.\n";
				return false;
			}
		}
		// limit to Players?
		if (type == tPlayer) {
			Player* player = MPlayer.get(name);
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
			Object* object = builder->clFindObject(name, GOC_ANY, true);
			if (object != NULL) {
				entity = object;
				return true;
			}
		}

		// find portal?
		if (type == tNone) {
			Portal* portal = builder->clFindPortal(name, true);
			if (portal != NULL) {
				entity = portal;
				return true;
			}
		}
		if (type == tPortal) {
			Room* room = builder->getRoom();
			if (!room) {
				*builder << "You are not in a room.\n";
				return false;
			}
			Portal* portal = builder->clFindPortal(name, true);
			if (portal == NULL) {
				*builder << "Cannot find portal '" << name << "'.\n";
				return false;
			}
			entity = portal;
			return true;
		}

		// find room?
		if (type == tRoom) {
			Room* room = NULL;
			if (!name.empty())
				room = MZone.getRoom(name);
			else
				room = builder->getRoom();
			if (room != NULL) {
				entity = room;
				return true;
			} else {
				*builder << "Cannot find room '" << name << "'.\n";
				return false;
			}
		}

		// find zone?
		/* FIXME
		if (type == tZone) {
			Zone* zone = NULL;
			if (!name.empty())
				zone = MZone.getZone(name);
			else
				zone = builder->getRoom() ? builder->getRoom()->getZone() : NULL;
			if (zone != NULL) {
				entity = zone;
				return true;
			} else {
				*builder << "Cannot find zone '" << name << "'.\n";
				return false;
			}
		}
		*/

		// none found
		*builder << "Cannot find '" << name << "'.\n";
		return false;
	}

	void
	doOlc(Player* user, OLCMode olc_mode, Entity* olc_entity, const std::string& olc_attr, const std::string& value)
	{
		bool olc_ok = false;

		// ----- BEGIN OLC BLOCK -----

		OLC_BEGIN_TYPE(Entity)
		OLC_BEGIN_ATTR(name)
		OLC_GET
		OLC_DISPLAY(edit->getName().getFull())
		OLC_END_ATTR
		OLC_BEGIN_ATTR(desc)
		OLC_GET
		OLC_DISPLAY(edit->getDesc())
		OLC_END_ATTR
		OLC_END_TYPE

		OLC_BEGIN_TYPE(Creature)
		OLC_BEGIN_ATTR(hp)
		OLC_GET
		OLC_DISPLAY(edit->getHP())
		OLC_SET
		edit->setHP(tolong(value));
		OLC_END_ATTR
		OLC_END_TYPE

		OLC_BEGIN_TYPE(Object)
		OLC_BEGIN_ATTR(blueprint)
		OLC_GET
		OLC_DISPLAY(edit->getBlueprint()->getId())
		OLC_SET
		ObjectBP* blueprint = MObjectBP.lookup(value);
		if (blueprint != NULL)
			edit->setBlueprint(blueprint);
		else
			*user << "Blueprint not found.\n";
		OLC_END_ATTR
		OLC_END_TYPE

		OLC_BEGIN_TYPE(Npc)
		OLC_BEGIN_ATTR(blueprint)
		OLC_GET
		OLC_DISPLAY(edit->getBlueprint()->getId())
		OLC_SET
		NpcBP* blueprint = MNpcBP.lookup(value);
		if (blueprint != NULL)
			edit->setBlueprint(blueprint);
		else
			*user << "Blueprint not found.\n";
		OLC_END_ATTR
		OLC_END_TYPE

		OLC_BEGIN_TYPE(Portal)
		OLC_END_TYPE

		// ----- END OLC BLOCK -----

		if (!olc_ok && olc_mode != OLC_MODE_LIST)
			*user << "The attribute '" << olc_attr << "' does not exist.\n";
	}
}
