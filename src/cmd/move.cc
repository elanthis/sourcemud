/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
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
void command_north(Creature* ch, std::string argv[])
{
	// must be in a room
	if (!ch->getRoom()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->getRoom()->getPortalByDir(PortalDir::NORTH);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->doGo(portal);
}

/* BEGIN COMMAND
 *
 * name: northeast
 *
 * format: northeast (12)
 * format: ne (12)
 *
 * END COMMAND */
void command_northeast(Creature* ch, std::string argv[])
{
	// must be in a room
	if (!ch->getRoom()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->getRoom()->getPortalByDir(PortalDir::NORTHEAST);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->doGo(portal);
}

/* BEGIN COMMAND
 *
 * name: east
 *
 * format: east (10)
 * format: e (10)
 *
 * END COMMAND */
void command_east(Creature* ch, std::string argv[])
{
	// must be in a room
	if (!ch->getRoom()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->getRoom()->getPortalByDir(PortalDir::EAST);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->doGo(portal);
}

/* BEGIN COMMAND
 *
 * name: southeast
 *
 * format: southeast (12)
 * format: se (12)
 *
 * END COMMAND */
void command_southeast(Creature* ch, std::string argv[])
{
	// must be in a room
	if (!ch->getRoom()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->getRoom()->getPortalByDir(PortalDir::SOUTHEAST);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->doGo(portal);
}

/* BEGIN COMMAND
 *
 * name: south
 *
 * format: south (10)
 * format: s (10)
 *
 * END COMMAND */
void command_south(Creature* ch, std::string argv[])
{
	// must be in a room
	if (!ch->getRoom()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->getRoom()->getPortalByDir(PortalDir::SOUTH);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->doGo(portal);
}

/* BEGIN COMMAND
 *
 * name: southwest
 *
 * format: southwest (12)
 * format: sw (12)
 *
 * END COMMAND */
void command_southwest(Creature* ch, std::string argv[])
{
	// must be in a room
	if (!ch->getRoom()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->getRoom()->getPortalByDir(PortalDir::SOUTHWEST);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->doGo(portal);
}

/* BEGIN COMMAND
 *
 * name: west
 *
 * format: west (10)
 * format: w (10)
 *
 * END COMMAND */
void command_west(Creature* ch, std::string argv[])
{
	// must be in a room
	if (!ch->getRoom()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->getRoom()->getPortalByDir(PortalDir::WEST);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->doGo(portal);
}

/* BEGIN COMMAND
 *
 * name: northwest
 *
 * format: northwest (12)
 * format: nw (12)
 *
 * END COMMAND */
void command_northwest(Creature* ch, std::string argv[])
{
	// must be in a room
	if (!ch->getRoom()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->getRoom()->getPortalByDir(PortalDir::NORTHWEST);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->doGo(portal);
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
	if (ch->getPosition() != CreaturePosition::STAND) {
		*ch << "You cannot climb while " << ch->getPosition().getState() << ".\n";
		return;
	}

	Portal* portal;
	if ((portal = ch->clFindPortal(argv[0])) != NULL) {
		if (portal->getUsage() == PortalUsage::CLIMB) {
			ch->doGo(portal);
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
	if (ch->getPosition() != CreaturePosition::KNEEL && ch->getPosition() != CreaturePosition::SIT) {
		*ch << "You cannot crawl while " << ch->getPosition().getState() << ".\n";
		return;
	}

	Portal* portal;
	if ((portal = ch->clFindPortal(argv[0])) != NULL) {
		if (portal->getUsage() == PortalUsage::CRAWL) {
			ch->doGo(portal);
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
	if (ch->getPosition() != CreaturePosition::STAND) {
		*ch << "You cannot climb while " << ch->getPosition().getState() << ".\n";
		return;
	}

	Portal* portal;
	if ((portal = ch->clFindPortal(argv[0])) != NULL) {
		if (portal->getUsage() == PortalUsage::WALK) {
			ch->doGo(portal);
		} else {
			*ch << "You cannot do that with " << StreamName(*portal, DEFINITE) << ".\n";
		}
	}
}
