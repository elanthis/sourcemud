/*
 * Source MUD
 * Copyright (C) 2008  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "mud/room.h"
#include "mud/portal.h"
#include "mud/object.h"
#include "mud/zone.h"
#include "mud/creature.h"
#include "mud/player.h"
#include "mud/npc.h"

namespace Hooks
{
	bool save_creature(Creature* self, File::Writer& writer) { return false; }
	bool creature_heartbeat(Creature* self) { return false; }
	bool save_entity(Entity* self, File::Writer& writer) { return false; }
	bool save_portal(Portal* self, File::Writer& writer) { return false; }
	bool ready() { return false; }
	bool change_hour() { return false; }
	bool save_npc(Npc* self, File::Writer& writer) { return false; }
	bool npc_death(Npc* self, Creature* killer) { return false; }
	bool npc_heartbeat(Npc* self) { return false; }
	bool save_object_blueprint(ObjectBP* self, File::Writer& swriter) { return false; }
	bool save_object(Object* self, File::Writer& writer) { return false; }
	bool object_heartbeat(Object* self) { return false; }
	bool save_player(Player* self, File::Writer& writer) { return false; }
	bool player_start(Player* self) { return false; }
	bool create_creature(Player* self) { return false; }
	bool player_death(Player* self, Creature* killer) { return false; }
	bool player_heartbeat(Player* self) { return false; }
	bool save_room(Room* self, File::Writer& writer) { return false; }
	bool room_heartbeat(Room* self) { return false; }
	bool show_room(Room* self, Creature* viewer) { return false; }
	bool save_zone(Zone* self, File::Writer& writer) { return false; }
}
