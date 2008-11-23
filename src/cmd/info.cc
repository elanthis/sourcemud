/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "mud/creature.h"
#include "mud/command.h"
#include "mud/player.h"
#include "mud/server.h"
#include "mud/macro.h"
#include "common/streams.h"
#include "common/mail.h"
#include "mud/color.h"
#include "mud/telnet.h"
#include "mud/settings.h"
#include "config.h"

/* BEGIN COMMAND
 *
 * name: affects
 *
 * format: affects
 *
 * END COMMAND */
void command_affects (Creature* ch, String argv[])
{
	ch->display_affects(*ch);
}

/* BEGIN COMMAND
 *
 * name: commands
 *
 * END COMMAND */
void command_commands (Player *ply, String[])
{
	CommandManager.show_list (ply);
}

/* BEGIN COMMAND
 *
 * name: man
 * usage: man <command>
 *
 * format: man :0*?
 *
 * END COMMAND */
void command_man (Player* player, String argv[])
{
	StreamControl stream(player);
	CommandManager.show_man(stream, argv[0]);
}

/* BEGIN COMMAND
 *
 * name: server
 *
 * END COMMAND */
void command_server (Player *player, String[])
{
	*player << "Source MUD V" PACKAGE_VERSION "\nBuild: " __DATE__ " " __TIME__ "\nUptime: " << MUD::get_uptime() << "\n";
}

/* BEGIN COMMAND
 *
 * name: skills
 *
 * END COMMAND */
void command_skills (Player* player, String[])
{
	player->display_skills ();
}

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

/* BEGIN COMMAND
 *
 * name: who
 *
 * END COMMAND */
void command_who (Player *player, String[])
{
	PlayerManager.list (*player);
}
