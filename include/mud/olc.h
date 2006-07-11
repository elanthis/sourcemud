/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef OLC_H
#define OLC_H

namespace OLC {
	// -- Lookup something to edit --
	// name will be modified
	// result stored in entity
	// returns false on error or not found
	bool lookup_editable (Player* builder, StringArg type, StringArg name, Entity*& entity);

	// -- ENTITY --
	bool modify_entity (Player* builder, Entity* entity, const char* attr, char* value);
	void show_entity (Player* builder, Entity* entity);

	// -- CHARACTER -- 
	bool modify_character (Player* builder, Character* ch, const char* attr, char* value);
	void show_character (Player* builder, Character* ch);

	// -- NPC --
	bool modify_npc (Player* builder, Npc* npc, const char* attr, char* value);
	void show_npc (Player* builder, Npc* npc);

	// -- PLAYER --
	bool modify_player (Player* builder, Player* player, const char* attr, char* value);
	void show_player (Player* builder, Player* player);

	// -- OBJECT --
	bool modify_object (Player* builder, Object* object, const char* attr, char* value);
	void show_object (Player* builder, Object* object);

	// -- ROOM --;
	bool modify_room (Player* builder, Room* room, const char* attr, char* value);
	void show_room (Player* builder, Room* room);

	// -- EXIT --
	bool modify_exit (Player* builder, RoomExit* exit, const char* attr, char* value);
	void show_exit (Player* builder, RoomExit* exit);

	// -- ZONE --
	bool modify_zone (Player* builder, Zone* zone, const char* attr, char* value);
	void show_zone (Player* builder, Zone* zone);
}

#endif
