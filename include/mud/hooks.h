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

namespace Hooks {
	bool save_creature(Creature* self, File::Writer& writer);
	bool creature_heartbeat(Creature* self);
	bool save_entity(Entity* self, File::Writer& writer);
	bool save_portal(Portal* self, File::Writer& writer);
	bool ready();
	bool change_hour();
	bool save_npc(Npc* self, File::Writer& writer);
	bool npc_death(Npc* self, Creature* killer);
	bool npc_heartbeat(Npc* self);
	bool save_object_blueprint(ObjectBP* self, File::Writer& swriter);
	bool save_object(Object* self, File::Writer& writer);
	bool object_heartbeat(Object* self);
	bool save_player(Player* self, File::Writer& writer);
	bool player_start(Player* self);
	bool create_creature(Player* self);
	bool player_death(Player* self, Creature* killer);
	bool player_heartbeat(Player* self);
	bool save_room(Room* self, File::Writer& writer);
	bool room_heartbeat(Room* self);
	bool show_room(Room* self, Creature* viewer);
	bool save_zone(Zone* self, File::Writer& writer);
}

#endif
