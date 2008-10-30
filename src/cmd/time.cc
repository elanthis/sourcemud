/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "mud/player.h"
#include "mud/server.h"
#include "mud/macro.h"
#include "mud/command.h"
#include "common/streams.h"
#include "common/mail.h"
#include "mud/color.h"
#include "mud/telnet.h"
#include "mud/settings.h"

/* BEGIN COMMAND
 *
 * name: time
 *
 * END COMMAND */

void command_time (Player *player, String[])
{
	char time_str[40];
	char date_str[120];
	TimeManager.time.time_str (time_str, sizeof (time_str));
	TimeManager.time.date_str (date_str, sizeof (date_str));
	*player << "It is currently " << time_str << " on " << date_str << ".  ";
	if (TimeManager.time.is_day ())
		*player << "It is daytime.  The Sun will set in " << (SUN_DOWN_HOUR - TimeManager.time.get_hour()) << " hours.\n";
	else
		*player << "It is nighttime.  The Sun will rise in " << ((TimeManager.time.get_hour() < SUN_UP_HOUR) ? SUN_UP_HOUR - TimeManager.time.get_hour() : SUN_UP_HOUR + 24 - TimeManager.time.get_hour()) << " hours.\n";
}

