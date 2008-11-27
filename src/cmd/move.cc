/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "mud/creature.h"
#include "mud/room.h"
#include "mud/command.h"

/* BEGIN COMMAND
 *
 * name: north
 *
 * format: north (10)
 * format: n (10)
 *
 * END COMMAND */
void command_north (Creature* ch, std::string argv[]) {
	// must be in a room
	if (!ch->get_room()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->get_room()->get_portal_by_dir (PortalDir::NORTH);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->do_go(portal);
}

/* BEGIN COMMAND
 *
 * name: northeast
 *
 * format: northeast (12)
 * format: ne (12)
 *
 * END COMMAND */
void command_northeast (Creature* ch, std::string argv[]) {
	// must be in a room
	if (!ch->get_room()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->get_room()->get_portal_by_dir (PortalDir::NORTHEAST);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->do_go(portal);
}

/* BEGIN COMMAND
 *
 * name: east
 *
 * format: east (10)
 * format: e (10)
 *
 * END COMMAND */
void command_east (Creature* ch, std::string argv[]) {
	// must be in a room
	if (!ch->get_room()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->get_room()->get_portal_by_dir (PortalDir::EAST);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->do_go(portal);
}

/* BEGIN COMMAND
 *
 * name: southeast
 *
 * format: southeast (12)
 * format: se (12)
 *
 * END COMMAND */
void command_southeast (Creature* ch, std::string argv[]) {
	// must be in a room
	if (!ch->get_room()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->get_room()->get_portal_by_dir (PortalDir::SOUTHEAST);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->do_go(portal);
}

/* BEGIN COMMAND
 *
 * name: south
 *
 * format: south (10)
 * format: s (10)
 *
 * END COMMAND */
void command_south (Creature* ch, std::string argv[]) {
	// must be in a room
	if (!ch->get_room()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->get_room()->get_portal_by_dir (PortalDir::SOUTH);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->do_go(portal);
}

/* BEGIN COMMAND
 *
 * name: southwest
 *
 * format: southwest (12)
 * format: sw (12)
 *
 * END COMMAND */
void command_southwest (Creature* ch, std::string argv[]) {
	// must be in a room
	if (!ch->get_room()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->get_room()->get_portal_by_dir (PortalDir::SOUTHWEST);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->do_go(portal);
}

/* BEGIN COMMAND
 *
 * name: west
 *
 * format: west (10)
 * format: w (10)
 *
 * END COMMAND */
void command_west (Creature* ch, std::string argv[]) {
	// must be in a room
	if (!ch->get_room()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->get_room()->get_portal_by_dir (PortalDir::WEST);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->do_go(portal);
}

/* BEGIN COMMAND
 *
 * name: northwest
 *
 * format: northwest (12)
 * format: nw (12)
 *
 * END COMMAND */
void command_northwest (Creature* ch, std::string argv[]) {
	// must be in a room
	if (!ch->get_room()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->get_room()->get_portal_by_dir (PortalDir::NORTHWEST);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->do_go(portal);
}

/* BEGIN COMMAND
 *
 * name: climb
 * usage: climb <portal>
 *
 * format: climb :0* (10)
 *
 * END COMMAND */
void command_climb(Creature* ch, std::string argv[])
{
	if (ch->get_pos() != CreaturePosition::STAND) {
		*ch << "You cannot climb while " << ch->get_pos().get_verbing() << ".\n";
		return;
	}

	Portal* portal;
	if ((portal = ch->cl_find_portal (argv[0])) != NULL) {
		if (portal->get_usage() == PortalUsage::CLIMB) {
			ch->do_go (portal);
		} else {
			*ch << "You cannot climb " << StreamName(*portal, DEFINITE) << ".\n";
		}
	}
}

/* BEGIN COMMAND
 *
 * name: crawl
 * usage: crawl <portal>
 *
 * format: crawl :0* (10)
 *
 * END COMMAND */
void command_crawl(Creature* ch, std::string argv[])
{
	if (ch->get_pos() != CreaturePosition::KNEEL && ch->get_pos() != CreaturePosition::SIT) {
		*ch << "You cannot crawl while " << ch->get_pos().get_verbing() << ".\n";
		return;
	}

	Portal* portal;
	if ((portal = ch->cl_find_portal (argv[0])) != NULL) {
		if (portal->get_usage() == PortalUsage::CRAWL) {
			ch->do_go (portal);
		} else {
			*ch << "You cannot crawl on " << StreamName(*portal, DEFINITE) << ".\n";
		}
	}
}

/* BEGIN COMMAND
 *
 * name: go
 * usage: go <portal>
 *
 * format: go :0*
 *
 * END COMMAND */
void command_go(Creature* ch, std::string argv[])
{
	if (ch->get_pos() != CreaturePosition::STAND) {
		*ch << "You cannot climb while " << ch->get_pos().get_verbing() << ".\n";
		return;
	}

	Portal* portal;
	if ((portal = ch->cl_find_portal (argv[0])) != NULL) {
		if (portal->get_usage() == PortalUsage::WALK) {
			ch->do_go (portal);
		} else {
			*ch << "You cannot do that with " << StreamName(*portal, DEFINITE) << ".\n";
		}
	}
}
