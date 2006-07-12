/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/player.h"
#include "mud/server.h"
#include "mud/parse.h"
#include "mud/command.h"
#include "common/streams.h"
#include "common/mail.h"
#include "mud/color.h"
#include "mud/telnet.h"
#include "mud/settings.h"

void
Player::process_command (String line)
{
	if (str_eq(line, S("quit")))
		quit();

	if (line.empty())
		return;

	if (line[0] == '#')
		line = last_command;
	else
		last_command = line;

	/* check for talking with \", a special case */
	if (line[0] == '\"' || line[0] == '\'')
	{
		if (line[1] == '\0')
			*this << "Say what?\n";
		else
			do_say (String(line.c_str() + 1)); // FIXME: make this more efficient
		return;
	}

	/* check for emote'ing with :, a special case */
	if (line[0] == ':' || line[0] == ';')
	{
		if (line[1] == '\0')
			*this << "Do what?\n";
		else
			do_emote (String(line.c_str() + 1)); // FIXME: make this more efficient
		return;
	}

	CommandManager.call (this, line);
}

void command_tell (Player* player, String argv[])
{
	Player* cn = PlayerManager.get(argv[0]);
	if (cn) {
		player->do_tell(cn, argv[1]);
	} else {
		*player << "Player '" << argv[0] << "' is not logged in.\n";
	}
}

void
Player::do_tell (Player* who, String what)
{
	*who << "[" << StreamName(this) << "]: " CTALK << what << CNORMAL "\n";
	who->last_tell = get_id();
	*this << "Message sent to " << StreamName(who) << ".\n";
}

void command_reply (Player* player, String argv[])
{
	player->do_reply(argv[0]);
}

void
Player::do_reply (String what)
{
	if (!last_tell) {
		*this << "No one has sent you a tell yet.\n";
		return;
	}

	Player* who = PlayerManager.get(last_tell);
	if (who) {
		*who << "[" << StreamName(this) << "]: " CTALK << what << CNORMAL "\n";
		who->last_tell = get_id();
		*this << "Reply sent to " << StreamName(who) << ".\n";
	} else {
		*this << "Player '" << last_tell << "' is not logged in.\n";
	}
}

void command_inventory (Player* player, String[])
{
	player->display_inventory ();
}

void command_skills (Player* player, String[])
{
	player->display_skills ();
}

void command_setcolor (Player* player, String argv[])
{
	// must have a telnet connection
	if (player->get_telnet() == NULL) {
		*player << "Must have an active telnet connection.\n";
		return;
	}

	// get type/color
	int ctype, cvalue;
	for (ctype = 0; !color_type_names[ctype].empty(); ++ctype)
		if (str_eq(color_type_names[ctype], argv[0]))
			break;
	for (cvalue = 0; !color_value_names[cvalue].empty(); ++cvalue)
		if (str_eq(color_value_names[cvalue], argv[1]))
			break;

	// invalid color type
	if (color_type_names[ctype].empty()) {
		// determine column count
		int max_col = (player->get_width() - 3) / 20; // figure max size
		if (max_col <= 0)
			max_col = 3;

		// print list
		*player << "Invalid type '" << argv[0] << "'.  Possible options are:\n";
		int col = 0;
		for (uint i = 0; !color_type_names[i].empty(); ++i) {
			// display
			player->set_indent(col * 20 + 2);
			*player << color_type_names[i];

			// move column
			if (++col >= max_col) {
				*player << "\n";
				col = 0;
			}
		}
		if (col)
			*player << "\n";
		player->set_indent(0);

	// invalid color
	} else if (color_value_names[cvalue].empty()) {
		// determine column count
		int max_col = (player->get_width() - 3) / 20; // figure max size
		if (max_col <= 0)
			max_col = 3;

		// print list
		*player << "Invalid color '" << argv[1] << "'.  Possible options are:\n";
		int col = 0;
		for (uint i = 0; !color_value_names[i].empty(); ++i) {
			// display
			player->set_indent(col * 20 + 2);
			*player << color_value_names[i] << " (" << color_values[i] << "ABC" << CNORMAL << ")";

			// move column
			if (++col >= max_col) {
				*player << "\n";
				col = 0;
			}
		}
		if (col)
			*player << "\n";
		player->set_indent(0);

	// set color
	} else {
		player->get_telnet()->set_color (ctype, cvalue);
		*player << "Color '" << color_type_names [ctype]<< "' set to " << color_values[cvalue] << color_value_names[cvalue] << CNORMAL "\n";
	}
}

void command_commands (Player *ply, String[])
{
	CommandManager.show_list (ply);
}

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

void command_who (Player *player, String[])
{
	PlayerManager.list (*player);
}

void command_server (Player *player, String[])
{
	*player << "AweMUD V" VERSION "\nBuild: " __DATE__ " " __TIME__ "\nUptime: " << AweMUD::get_uptime() << "\n";
}

void command_bug (Player* player, String argv[])
{
#ifdef HAVE_SENDMAIL
	// mail address
	String rcpt = SettingsManager.get_bugs_email();
	if (!rcpt) {
		*player << CADMIN "Bug reporting has been disabled." CNORMAL "\n";
		return;
	}

	// sent bug report
	StringBuffer body;
	body << "# -- HEADER --\n";
	body << "Issue: BUG\n";
	body << "Host: " << NetworkManager.get_host() << "\n";
	body << "From: " << player->get_account()->get_id() << "\n";
	body << "# -- END --\n";
	body << "# -- BODY --\n";
	body << argv[0] << "\n";
	body << "# -- END --\n";
	body << '\0';
	MailMessage msg (rcpt, "Bug Report", body.str());
	msg.send();

	// send message 
	Log::Info << "Player " << player->get_account()->get_id() << " issued a bug report.";
	*player << "Your bug report has been sent.\n";
#else // HAVE_SENDMAIL
	*player << CADMIN "Bug reporting has been disabled." CNORMAL "\n";
#endif // HAVE_SENDMAIL
}

void command_abuse (Player* player, String argv[])
{
#ifdef HAVE_SENDMAIL
	// mail address
	String rcpt = SettingsManager.get_abuse_email();
	if (!rcpt) {
		*player << CADMIN "Abuse reporting has been disabled." CNORMAL "\n";
		return;
	}

	// sent bug report
	StringBuffer body;
	body << "# -- HEADER --\n";
	body << "Issue: ABUSE\n";
	body << "Host: " << NetworkManager.get_host() << "\n";
	body << "From: " << player->get_account()->get_id() << "\n";
	body << "About: " << argv[0] << "\n";
	body << "# -- END --\n";
	body << "# -- BODY --\n";
	body << argv[1] << "\n";
	body << "# -- END --\n";
	body << '\0';
	MailMessage msg (rcpt, "Abuse Report", body.str());
	msg.send();

	// send message 
	Log::Info << "Player " << player->get_account()->get_id() << " issued an abuse report.";
	*player << "Your abuse report has been sent.\n";
#else // HAVE_SENDMAIL
	*player << CADMIN "Bug reporting has been disabled." CNORMAL "\n";
#endif // HAVE_SENDMAIL
}

void command_man (Player* player, String argv[])
{
	StreamControl stream(player);
	CommandManager.show_man(stream, argv[0]);
}
