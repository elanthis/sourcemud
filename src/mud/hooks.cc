/*
 * Source MUD
 * Copyright (C) 2008  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "lua/exec.h"
#include "mud/room.h"
#include "mud/portal.h"
#include "mud/object.h"
#include "mud/zone.h"
#include "mud/creature.h"
#include "mud/player.h"
#include "mud/npc.h"

namespace Hooks {

bool saveCreature(Creature* self, File::Writer& writer)
{
	Lua::ExecHook exec("save_creature");
	exec.param(self);
	Lua::createObject(&writer, "FileWriter");
	exec.param(&writer);
	exec.run();
	Lua::releaseObject(&writer);
	return exec.getBoolean();
}

bool creatureHeartbeat(Creature* self)
{
	Lua::ExecHook exec("creature_heartbeat");
	exec.param(self);
	exec.run();
	return exec.getBoolean();
}

bool saveEntity(Entity* self, File::Writer& writer)
{
	Lua::ExecHook exec("save_entity");
	exec.param(self);
	Lua::createObject(&writer, "FileWriter");
	exec.param(&writer);
	exec.run();
	Lua::releaseObject(&writer);
	return exec.getBoolean();
}

bool savePortal(Portal* self, File::Writer& writer)
{
	Lua::ExecHook exec("save_portal");
	exec.param(self);
	Lua::createObject(&writer, "FileWriter");
	exec.param(&writer);
	exec.run();
	Lua::releaseObject(&writer);
	return exec.getBoolean();
}

bool ready()
{
	Lua::ExecHook exec("ready");
	exec.run();
	return exec.getBoolean();
}

bool changeHour()
{
	Lua::ExecHook exec("change_hour");
	exec.run();
	return exec.getBoolean();
}

bool saveNpc(Npc* self, File::Writer& writer)
{
	Lua::ExecHook exec("save_npc");
	exec.param(self);
	Lua::createObject(&writer, "FileWriter");
	exec.param(&writer);
	exec.run();
	Lua::releaseObject(&writer);
	return exec.getBoolean();
}

bool npcDeath(Npc* self, Creature* killer)
{
	Lua::ExecHook exec("npc_death");
	exec.param(self);
	exec.param(killer);
	exec.run();
	return exec.getBoolean();
}

bool npcHeartbeat(Npc* self)
{
	Lua::ExecHook exec("npc_heartbeat");
	exec.param(self);
	exec.run();
	return exec.getBoolean();
}

bool saveObjectBlueprint(ObjectBP* self, File::Writer& writer)
{
	Lua::ExecHook exec("save_object_blueprint");
	exec.param(self);
	Lua::createObject(&writer, "FileWriter");
	exec.param(&writer);
	exec.run();
	Lua::releaseObject(&writer);
	return exec.getBoolean();
}

bool saveObject(Object* self, File::Writer& writer)
{
	Lua::ExecHook exec("save_object");
	exec.param(self);
	Lua::createObject(&writer, "FileWriter");
	exec.param(&writer);
	exec.run();
	Lua::releaseObject(&writer);
	return exec.getBoolean();
}

bool objectHeartbeat(Object* self)
{
	Lua::ExecHook exec("object_heartbeat");
	exec.param(self);
	exec.run();
	return exec.getBoolean();
}

bool savePlayer(Player* self, File::Writer& writer)
{
	Lua::ExecHook exec("save_player");
	exec.param(self);
	Lua::createObject(&writer, "FileWriter");
	exec.param(&writer);
	exec.run();
	Lua::releaseObject(&writer);
	return exec.getBoolean();
}

bool playerStart(Player* self)
{
	Lua::ExecHook exec("player_start");
	exec.param(self);
	exec.run();
	return exec.getBoolean();
}

bool createCreature(Player* self)
{
	Lua::ExecHook exec("create_creature");
	exec.param(self);
	exec.run();
	return exec.getBoolean();
}

bool playerDeath(Player* self, Creature* killer)
{
	Lua::ExecHook exec("player_death");
	exec.param(self);
	exec.param(killer);
	exec.run();
	return exec.getBoolean();
}

bool playerHeartbeat(Player* self)
{
	Lua::ExecHook exec("player_heartbeat");
	exec.param(self);
	exec.run();
	return exec.getBoolean();
}

bool saveRoom(Room* self, File::Writer& writer)
{
	Lua::ExecHook exec("save_room");
	exec.param(self);
	Lua::createObject(&writer, "FileWriter");
	exec.param(&writer);
	exec.run();
	Lua::releaseObject(&writer);
	return exec.getBoolean();
}

bool roomHeartbeat(Room* self)
{
	Lua::ExecHook exec("room_heartbeat");
	exec.param(self);
	exec.run();
	return exec.getBoolean();
}

bool showRoom(Room* self, Creature* viewer)
{
	Lua::ExecHook exec("show_room");
	exec.param(self);
	exec.param(viewer);
	exec.run();
	return exec.getBoolean();
}

bool saveZone(Zone* self, File::Writer& writer)
{
	Lua::ExecHook exec("save_zone");
	exec.param(self);
	Lua::createObject(&writer, "FileWriter");
	exec.param(&writer);
	exec.run();
	Lua::releaseObject(&writer);
	return exec.getBoolean();
}


} // namespace Hooks
