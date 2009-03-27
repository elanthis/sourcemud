/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/streams.h"
#include "common/mail.h"
#include "mud/creature.h"
#include "mud/command.h"
#include "mud/player.h"
#include "mud/server.h"
#include "mud/macro.h"
#include "mud/color.h"
#include "mud/settings.h"
#include "net/telnet.h"

/* BEGIN COMMAND
 *
 * name: affects
 *
 * format: affects
 *
 * END COMMAND */
void command_affects(Creature* ch, std::string argv[])
{
	ch->displayAffects(*ch);
}

/* BEGIN COMMAND
 *
 * name: commands
 *
 * END COMMAND */
void command_commands(Player *ply, std::string[])
{
	MCommand.showList(ply);
}

/* BEGIN COMMAND
 *
 * name: man
 * usage: man <command>
 *
 * format: man :0*?
 *
 * END COMMAND */
void command_man(Player* player, std::string argv[])
{
	StreamControl stream(player);
	MCommand.showMan(stream, argv[0]);
}

/* BEGIN COMMAND
 *
 * name: server
 *
 * END COMMAND */
void command_server(Player *player, std::string[])
{
	*player << "Source MUD V" PACKAGE_VERSION "\nBuild: " __DATE__ " " __TIME__ "\nUptime: " << MUD::getUptime() << "\n";
}

/* BEGIN COMMAND
 *
 * name: skills
 *
 * END COMMAND */
void command_skills(Player* player, std::string[])
{
	player->displaySkills();
}

/* BEGIN COMMAND
 *
 * name: time
 *
 * END COMMAND */
void command_time(Player *player, std::string[])
{
	char time_str[40];
	char date_str[120];
	MTime.time.timeStr(time_str, sizeof(time_str));
	MTime.time.dateStr(date_str, sizeof(date_str));
	*player << "It is currently " << time_str << " on " << date_str << ".  ";
	if (MTime.time.isDay())
		*player << "It is daytime.  The Sun will set in " << (SUN_DOWN_HOUR - MTime.time.getHour()) << " hours.\n";
	else
		*player << "It is nighttime.  The Sun will rise in " << ((MTime.time.getHour() < SUN_UP_HOUR) ? SUN_UP_HOUR - MTime.time.getHour() : SUN_UP_HOUR + 24 - MTime.time.getHour()) << " hours.\n";
}

/* BEGIN COMMAND
 *
 * name: who
 *
 * END COMMAND */
void command_who(Player *player, std::string[])
{
	MPlayer.list(*player);
}
