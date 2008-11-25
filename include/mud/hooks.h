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
class ScriptRestrictedWriter;
class ObjectBP;

namespace Hooks {
	bool save_creature(Creature* self, ScriptRestrictedWriter* writer);
	bool creature_heartbeat(Creature* self);
	bool save_entity(Entity* self, ScriptRestrictedWriter* writer);
	bool save_portal(Portal* self, ScriptRestrictedWriter* writer);
	bool ready();
	bool change_hour();
	bool save_npc(Npc* self, ScriptRestrictedWriter* writer);
	bool npc_death(Npc* self, Creature* killer);
	bool npc_heartbeat(Npc* self);
	bool save_object_blueprint(ObjectBP* self, ScriptRestrictedWriter* swriter);
	bool save_object(Object* self, ScriptRestrictedWriter* writer);
	bool object_heartbeat(Object* self);
	bool save_player(Player* self, ScriptRestrictedWriter* writer);
	bool player_start(Player* self);
	bool create_creature(Player* self);
	bool player_death(Player* self, Creature* killer);
	bool player_heartbeat(Player* self);
	bool save_room(Room* self, ScriptRestrictedWriter* writer);
	bool room_heartbeat(Room* self);
	bool show_room(Room* self, Creature* viewer);
	bool save_zone(Zone* self, ScriptRestrictedWriter* writer);
}

#endif
