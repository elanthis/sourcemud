/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/creature.h"
#include "common/error.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/command.h"
#include "mud/body.h"
#include "mud/player.h"
#include "mud/macro.h"
#include "common/streams.h"
#include "mud/object.h"

/* BEGIN COMMAND
 *
 * name: go
 * usage: go <portal>
 *
 * format: go :0*
 *
 * END COMMAND */

void command_go (Creature* ch, String argv[])
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
