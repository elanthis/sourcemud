/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef OLC_H
#define OLC_H

namespace OLC
{
	// -- Lookup something to edit --
	// name will be modified
	// result stored in entity
	// returns false on error or not found
	bool lookupEditable(Player* builder, const std::string& type, const std::string& name, Entity*& entity);

	// -- ENTITY --
	bool modifyEntity(Player* builder, Entity* entity, const char* attr, char* value);
	void showEntity(Player* builder, Entity* entity);

	// -- CHARACTER --
	bool modifyCharacter(Player* builder, Creature* ch, const char* attr, char* value);
	void showCharacter(Player* builder, Creature* ch);

	// -- NPC --
	bool modifyNpc(Player* builder, Npc* npc, const char* attr, char* value);
	void showNpc(Player* builder, Npc* npc);

	// -- PLAYER --
	bool modifyPlayer(Player* builder, Player* player, const char* attr, char* value);
	void showPlayer(Player* builder, Player* player);

	// -- OBJECT --
	bool modifyObject(Player* builder, Object* object, const char* attr, char* value);
	void showObject(Player* builder, Object* object);

	// -- ROOM --;
	bool modifyRoom(Player* builder, Room* room, const char* attr, char* value);
	void showRoom(Player* builder, Room* room);

	// -- EXIT --
	bool modifyPortal(Player* builder, Portal* portal, const char* attr, char* value);
	void showPortal(Player* builder, Portal* portal);

	// -- ZONE --
	bool modifyZone(Player* builder, Zone* zone, const char* attr, char* value);
	void showZone(Player* builder, Zone* zone);
}

#endif
