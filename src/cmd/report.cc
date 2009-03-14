/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/streams.h"
#include "common/mail.h"
#include "mud/player.h"
#include "mud/server.h"
#include "mud/macro.h"
#include "mud/command.h"
#include "mud/color.h"
#include "mud/settings.h"
#include "net/manager.h"
#include "net/telnet.h"

/* BEGIN COMMAND
 *
 * name: report abuse
 * usage: report abuse <message>
 *
 * format: report abuse :0* (60)
 *
 * END COMMAND */
void command_report_abuse(Player* player, std::string argv[])
{
#ifdef HAVE_SENDMAIL
	// mail address
	std::string rcpt = MSettings.get_abuse_email();
	if (rcpt.empty()) {
		*player << CADMIN "Abuse reporting has been disabled." CNORMAL "\n";
		return;
	}

	// sent bug report
	StringBuffer body;
	body << "# -- HEADER --\n";
	body << "Issue: ABUSE\n";
	body << "Host: " << MNetwork.get_host() << "\n";
	body << "From: " << player->get_account()->getId() << "\n";
	body << "About: " << argv[0] << "\n";
	body << "# -- END --\n";
	body << "# -- BODY --\n";
	body << argv[1] << "\n";
	body << "# -- END --\n";
	body << '\0';
	MailMessage msg(rcpt, "Abuse Report", body.str());
	msg.send();

	// send message
	Log::Info << "Player " << player->get_account()->getId() << " issued an abuse report.";
	*player << "Your abuse report has been sent.\n";
#else // HAVE_SENDMAIL
	*player << CADMIN "Bug reporting has been disabled." CNORMAL "\n";
#endif // HAVE_SENDMAIL
}

/* BEGIN COMMAND
 *
 * name: report bug
 * usage: report bug <message>
 *
 * format: report bug :0* (60)
 *
 * END COMMAND */
void command_report_bug(Player* player, std::string argv[])
{
#ifdef HAVE_SENDMAIL
	// mail address
	std::string rcpt = MSettings.get_bugs_email();
	if (rcpt.empty()) {
		*player << CADMIN "Bug reporting has been disabled." CNORMAL "\n";
		return;
	}

	// sent bug report
	StringBuffer body;
	body << "# -- HEADER --\n";
	body << "Issue: BUG\n";
	body << "Host: " << MNetwork.get_host() << "\n";
	body << "From: " << player->get_account()->getId() << "\n";
	body << "# -- END --\n";
	body << "# -- BODY --\n";
	body << argv[0] << "\n";
	body << "# -- END --\n";
	body << '\0';
	MailMessage msg(rcpt, "Bug Report", body.str());
	msg.send();

	// send message
	Log::Info << "Player " << player->get_account()->getId() << " issued a bug report.";
	*player << "Your bug report has been sent.\n";
#else // HAVE_SENDMAIL
	*player << CADMIN "Bug reporting has been disabled." CNORMAL "\n";
#endif // HAVE_SENDMAIL
}
