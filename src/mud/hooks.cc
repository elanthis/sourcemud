#include "mud/room.h"
#include "mud/portal.h"
#include "mud/object.h"
#include "mud/zone.h"
#include "mud/creature.h"
#include "mud/player.h"
#include "mud/npc.h"
#include "mud/shadow-object.h"

namespace Hooks {
	bool save_creature(Creature* self, ScriptRestrictedWriter* writer) { return false; }
	bool creature_heartbeat(Creature* self) { return false; }
	bool save_entity(Entity* self, ScriptRestrictedWriter* writer) { return false; }
	bool save_portal(Portal* self, ScriptRestrictedWriter* writer) { return false; }
	bool ready() { return false; }
	bool change_hour() { return false; }
	bool save_npc(Npc* self, ScriptRestrictedWriter* writer) { return false; }
	bool npc_death(Npc* self, Creature* killer) { return false; }
	bool npc_heartbeat(Npc* self) { return false; }
	bool save_object_blueprint(ObjectBP* self, ScriptRestrictedWriter* swriter) { return false; }
	bool save_object(Object* self, ScriptRestrictedWriter* writer) { return false; }
	bool object_heartbeat(Object* self) { return false; }
	bool save_player(Player* self, ScriptRestrictedWriter* writer) { return false; }
	bool player_start(Player* self) { return false; }
	bool create_creature(Player* self) { return false; }
	bool player_death(Player* self, Creature* killer) { return false; }
	bool player_heartbeat(Player* self) { return false; }
	bool save_room(Room* self, ScriptRestrictedWriter* writer) { return false; }
	bool room_heartbeat(Room* self) { return false; }
	bool show_room(Room* self, Creature* viewer) { return false; }
	bool save_zone(Zone* self, ScriptRestrictedWriter* writer) { return false; }
}
