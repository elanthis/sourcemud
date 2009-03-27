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
	bool saveCreature(Creature* self, File::Writer& writer) { return false; }
	bool creatureHeartbeat(Creature* self) { return false; }
	bool saveEntity(Entity* self, File::Writer& writer) { return false; }
	bool savePortal(Portal* self, File::Writer& writer) { return false; }
	bool ready() { return false; }
	bool changeHour() { return false; }
	bool saveNpc(Npc* self, File::Writer& writer) { return false; }
	bool npcDeath(Npc* self, Creature* killer) { return false; }
	bool npcHeartbeat(Npc* self) { return false; }
	bool saveObjectBlueprint(ObjectBP* self, File::Writer& swriter) { return false; }
	bool saveObject(Object* self, File::Writer& writer) { return false; }
	bool objectHeartbeat(Object* self) { return false; }
	bool savePlayer(Player* self, File::Writer& writer) { return false; }
	bool playerStart(Player* self) { return false; }
	bool createCreature(Player* self) { return false; }
	bool playerDeath(Player* self, Creature* killer) { return false; }
	bool playerHeartbeat(Player* self) { return false; }
	bool saveRoom(Room* self, File::Writer& writer) { return false; }
	bool roomHeartbeat(Room* self) { return false; }
	bool showRoom(Room* self, Creature* viewer) { return false; }
	bool saveZone(Zone* self, File::Writer& writer) { return false; }
}
