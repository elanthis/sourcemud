/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "char.h"
#include "error.h"
#include "server.h"
#include "room.h"
#include "command.h"
#include "player.h"
#include "streams.h"
#include "color.h"
#include "zone.h"
#include "telnet.h"

void command_gm_announce (Player* gm, char** argv)
{
	ZoneManager.announce (String(CADMIN "Announcement: " CNORMAL) + String(argv[0]));
}

void command_gm_boot (Player* gm, char** argv)
{
	Player *cn = PlayerManager.get(argv[0]);
	if (cn == gm) {
		*gm << "You cannot boot yourself.\n";
	} else if (cn == NULL) {
		*gm << "Player '" << argv[0] << "' is not logged in.\n";
	} else if (cn->get_telnet() == NULL) {
		*gm << "Player '" << argv[0] << "' is not actively connected.\n";
	} else {
		if (cn->get_telnet()) {
			*gm << "Booting " << StreamName(cn) << "...\n";
			*cn << CADMIN "You are being booted by a GM." CNORMAL "\n";
			Log::Info << "User " << cn->get_name () << " booted by " << gm->get_name ();
			cn->get_telnet()->disconnect();
		} else {
			*gm << "User " << cn->get_name() << " is already disconnected.\n";
		}
	}
}

void command_gm_teleport_room (Player* gm, char** argv)
{
	Room *r = ZoneManager.get_room (argv[0]);
	if (r) {
		*gm << "Jumping to " << r->get_name () << ".\n";
		gm->enter (r, NULL);
	} else
		*gm << "Could not find room '" << argv[0] << "'.\n";
}

void command_gm_teleport_player (Player* gm, char** argv)
{
	Player* player = PlayerManager.get(argv[0]);
	if (player == gm) {
		*gm << "You cannot teleport to yourself.\n";
	} else if (player) {
		if (player->get_room()) {
			*gm << "Jumping to player " << StreamName(player) << ".\n";
			gm->enter (player->get_room(), NULL);
		} else {
			*gm << "Player " << StreamName(player) << " isn't in a room.\n";
		}
	} else {
		*gm << "Could not find player '" << argv[0] << "'.\n";
	}
}

void command_gm_summon_player (Player* gm, char** argv)
{
	// check we have a room
	if (!gm->get_room()) {
		*gm << "You are not in a room.\n";
		return;
	}

	// find the player
	Player* player = PlayerManager.get(argv[0]);
	if (player == gm) {
		*gm << "You cannot summon yourself.\n";
	} else if (player) {
		*gm << "Summoning " << StreamName(player) << ".\n";
		player->enter (gm->get_room(), NULL);
		*player << CADMIN << "You have been summoned by " << StreamName(gm) << ".\n";
	} else {
		*gm << "Could not find player '" << argv[0] << "'.\n";
	}
}
