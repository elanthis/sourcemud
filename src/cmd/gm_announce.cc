/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "mud/creature.h"
#include "common/error.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/command.h"
#include "mud/player.h"
#include "common/streams.h"
#include "mud/color.h"
#include "mud/zone.h"
#include "mud/telnet.h"

/* BEGIN COMMAND
 *
 * name: gm announce
 * usage: gm announce <message>
 *
 * format: gm announce :0* (80)
 *
 * access: GM
 *
 * END COMMAND */

void command_gm_announce (Player* gm, String argv[])
{
	ZoneManager.announce (String(CADMIN "Announcement: " CNORMAL) + String(argv[0]));
}
