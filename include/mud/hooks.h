/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_HOOKS_H
#define SOURCEMUD_MUD_HOOKS_H

class Room;
class Entity;
class Object;
class Portal;
class Character;
class Player;
class Npc;
class Zone;
class ObjectBP;
namespace File { class Writer; }

namespace Hooks
{
	bool saveCreature(Creature* self, File::Writer& writer);
	bool creatureHeartbeat(Creature* self);
	bool saveEntity(Entity* self, File::Writer& writer);
	bool savePortal(Portal* self, File::Writer& writer);
	bool ready();
	bool changeHour();
	bool saveNpc(Npc* self, File::Writer& writer);
	bool npcDeath(Npc* self, Creature* killer);
	bool npcHeartbeat(Npc* self);
	bool saveObjectBlueprint(ObjectBP* self, File::Writer& swriter);
	bool saveObject(Object* self, File::Writer& writer);
	bool objectHeartbeat(Object* self);
	bool savePlayer(Player* self, File::Writer& writer);
	bool playerStart(Player* self);
	bool createCreature(Player* self);
	bool playerDeath(Player* self, Creature* killer);
	bool playerHeartbeat(Player* self);
	bool saveRoom(Room* self, File::Writer& writer);
	bool roomHeartbeat(Room* self);
	bool showRoom(Room* self, Creature* viewer);
	bool saveZone(Zone* self, File::Writer& writer);
}

#endif
