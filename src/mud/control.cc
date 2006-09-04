/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>

#include <algorithm>

#include "mud/control.h"
#include "common/log.h"
#include "mud/player.h"
#include "mud/server.h"
#include "mud/account.h"
#include "mud/zone.h"
#include "mud/settings.h"

SControlManager ControlManager;

#define MAX_CTRL_ARGS 20

#define CHECK_ADMIN \
	if (!is_admin()) { \
		*this << "+NOACCESS Access Denied\n"; \
		return; \
	} \

ControlHandler::ControlHandler (int s_sock, uid_t s_uid) : SocketUser(s_sock), account(NULL), uid(s_uid)
{
	in_buffer[0] = '\0';
}

bool
ControlHandler::is_admin () const
{
	return ControlManager.has_admin_access(uid) || (account != NULL && account->is_admin());
}

void
ControlHandler::in_handle (char* buffer, size_t size)
{
	int len = strlen(in_buffer);

	if (len + size + 1 > CONTROL_BUFFER_SIZE) {
		*this << "+INTERNAL Input overflow";
		close(sock);
		sock = -1;
		return;
	}

	memcpy(in_buffer + len, buffer, size);
	in_buffer[len + size] = 0;

	process();
}

void
ControlHandler::out_ready ()
{
	// FIXME: might lose output
	if (send(sock, out_buffer.c_str(), out_buffer.size(), 0) == -1) {
		Log::Error << "send() failed: " << strerror(errno);
		close(sock);
		sock = -1;
	}

	out_buffer.clear();
}

char
ControlHandler::get_poll_flags ()
{
	char flags = POLLSYS_READ;
	if (!out_buffer.empty())
		flags |= POLLSYS_WRITE;
	return flags;
}

void
ControlHandler::hangup ()
{
	Log::Network << "Control client disconnected";
}

void
ControlHandler::stream_put (const char* str, size_t len)
{
	out_buffer = out_buffer + String(str, len);
}

void
ControlHandler::process ()
{
	int len = strlen(in_buffer);

	// process lines
	char* brk = strchr(in_buffer, '\n');
	while (brk != NULL) {
		size_t blen = brk - in_buffer;

		// setup
		int argc = 0;
		String argv[MAX_CTRL_ARGS];
		argv[0] = String(in_buffer);
		char* cptr = in_buffer;
		char* bptr = in_buffer;
		bool quote = false;

		// process
		while (true) {
			// escape?  handle
			if (*cptr == '\\') {
				++cptr;
				// eol?  grr
				if (*cptr == '\n')
					break;
				// newline?
				else if (*cptr == 'n')
					*(bptr++) = '\n';
				// normal escape
				else
					*(bptr++) = *cptr;
			// quoted?  handle specially
			} else if (quote && *cptr) {
				// end quote?
				if (*cptr == '"')
					quote = false;
				// normal char
				else
					*(bptr++) = *cptr;
			// non-quoted space?  end of arg
			} else if (!quote && *cptr == ' ') {
				*bptr = 0;
				++argc;
				// out of argv?
				if (argc == 20)
					break;
				// increment arg pointer
				argv[argc] = String(++bptr);
				// skip multiple whitespace
				while (cptr[1] == ' ')
					++cptr;
				// end?  break now
				if (cptr[1] == '\n')
					break;
			// end of input?
			} else if (*cptr == '\n') {
				*bptr = 0;
				++argc;
				break;
			// non-quoted " ?  begin quote
			} else if (!quote && *cptr == '"') {
				quote = true;
			// normal char
			} else {
				*(bptr++) = *cptr;
			}

			// next
			++cptr;
		}

		// process
		handle(argc, argv);

		// cleanup
		memmove(in_buffer, in_buffer + blen + 1, len - (blen + 1));
		in_buffer[len - (blen + 1)] = 0;
		len = strlen(in_buffer);

		// loop
		brk = strchr(in_buffer, '\n');
	}
}

void
ControlHandler::handle (int argc, String argv[])
{
	// server version
	if (str_eq(argv[0], S("version"))) {
		*this << "-" << VERSION << "\n+OK\n";
	// server build
	} else if (str_eq(argv[0], S("build"))) {
		*this << "-" << __DATE__ << " " << __TIME__ << "\n+OK\n";
	// account count
	} else if (str_eq(argv[0], S("pcount"))) {
		*this << "-" << PlayerManager.count() << "\n+OK\n";
	// quit
	} else if (str_eq(argv[0], S("quit"))) {
		*this << "+OK Farewell\n";
		close(sock);
		sock = -1;
	// change passphrase
	} else if (str_eq(argv[0], S("chpass"))) {
		if (argc != 3) {
			*this << "+BADPARAM chpass <account> <pass>\n";
			return;
		}

		CHECK_ADMIN

		// check account exists
		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		// set passphrase
		account->set_passphrase(argv[2]);

		Log::Info << "Password of account '" << account->get_id() << "' changed over control interface";
		*this << "+OK Password changed\n";
	// new account
	} else if (str_eq(argv[0], S("newaccount"))) {
		// check args
		if (argc != 2) {
			*this << "+BADPARAM newaccount <account>\n";
			return;
		}

		CHECK_ADMIN

		// check accountname
		if (!AccountManager.valid_name(argv[1])) {
			*this << "+INVALID Invalid characters in account name or name too short or long\n";
			return;
		}

		// account exists?
		if (AccountManager.get(argv[1]) != NULL) {
			*this << "+DUPLICATE Account already exists\n";
			return;
		}

		// make the account
		if (AccountManager.create(argv[1]) == NULL) {
			*this << "+INTERNAL Failed to create new account\n";
			return;
		}
		Log::Info << "New account '" << argv[1] << "' created over control interface";

		*this << "+OK\n";
	// change name
	} else if (str_eq(argv[0], S("chname"))) {
		if (argc < 3) {
			*this << "+BADPARAM chname <account> <name>\n";
			return;
		}

		CHECK_ADMIN

		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		account->set_name(argv[2]);

		Log::Info << "Real name of account '" << account->get_id() << "' changed over control interface";
		*this << "+OK Name changed\n";
	// change email
	} else if (str_eq(argv[0], S("chmail"))) {
		if (argc != 3) {
			*this << "+BADPARAM chmail <account> <mail address>\n";
			return;
		}

		CHECK_ADMIN

		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		account->set_email(argv[2]);

		Log::Info << "E-mail address of account '" << account->get_name() << "' changed over control interface";
		*this << "+OK Mail address changed\n";
	// disable an account
	} else if (str_eq(argv[0], S("disable"))) {
		if (argc < 2) {
			*this << "+BADPARAM disable <account>\n";
			return;
		}

		CHECK_ADMIN

		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		account->set_disabled(true);
		account->save();

		Log::Info << "Account '" << account->get_id() << "' disabled over control interface";
		*this << "+OK Account disabled\n";
	// enable an account
	} else if (str_eq(argv[0], S("enable"))) {
		if (argc < 2) {
			*this << "+BADPARAM enable <account>\n";
			return;
		}

		CHECK_ADMIN

		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		account->set_disabled(false);
		account->save();

		Log::Info << "Account '" << account->get_id() << "' enabled over control interface";
		*this << "+OK Account enabled\n";
	// set max characters for an account
	} else if (str_eq(argv[0], S("setmaxcharacters"))) {
		if (argc < 3) {
			*this << "+BADPARAM setmaxcharacters <account> <amount>\n";
			return;
		}

		CHECK_ADMIN

		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		int amount = tolong(argv[2]);
		if (amount < 0) {
			*this << "+BADPARAM <amount> must be zero or greater\n";
			return;
		}

		account->set_max_characters(amount);
		account->save();

		Log::Info << "Account '" << account->get_id() << "' has max characters set to " << amount << " over control interface";
		*this << "+OK Account updated\n";
	// set max active for an account
	} else if (str_eq(argv[0], S("setmaxactive"))) {
		if (argc < 3) {
			*this << "+BADPARAM setmaxactive <account> <amount>\n";
			return;
		}

		CHECK_ADMIN

		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		int amount = tolong(argv[2]);
		if (amount < 0) {
			*this << "+BADPARAM <amount> must be zero or greater\n";
			return;
		}

		account->set_max_active(amount);
		account->save();

		Log::Info << "Account '" << account->get_id() << "' has max active set to " << amount << " over control interface";
		*this << "+OK Account updated\n";
	// show account info
	} else if (str_eq(argv[0], S("showaccount"))) {
		if (argc < 2) {
			*this << "+BADPARAM showaccount <account>\n";
			return;
		}

		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		*this << "-ID=" << account->get_id() << "\n";
		*this << "-NAME=" << account->get_name() << "\n";
		*this << "-EMAIL=" << account->get_email() << "\n";
		*this << "-MAXCHARS=" << account->get_max_characters() << "\n";
		*this << "-MAXACTIVE=" << account->get_max_active() << "\n";
		*this << "-DISABLED=" << (account->is_disabled() ? S("YES") : S("NO")) << "\n";
		*this << "-CHARACTERS=" << implode(account->get_char_list(), ',') << "\n";
		*this << "+OK\n";
	// shutdown server
	} else if (str_eq(argv[0], S("shutdown"))) {
		CHECK_ADMIN

		AweMUD::shutdown();
		*this << "+OK Shutting down\n";
	// announce
	} else if (str_eq(argv[0], S("announce"))) {
		if (argc < 2) {
			*this << "+BADPARAM announce <text>\n";
			return;
		}

		CHECK_ADMIN

		ZoneManager.announce(argv[1]);

		*this << "+OK\n";
	// connection list
	} else if (str_eq(argv[0], S("connections"))) {
		const IPConnList::ConnList& conn_list = NetworkManager.connections.get_conn_list();

		for (IPConnList::ConnList::const_iterator i = conn_list.begin(); i != conn_list.end(); ++i)
			*this << '-' << Network::get_addr_name(i->addr) << '\t' << i->conns << '\n';

		*this << "+OK\n";
	// attempt to login
	} else if (str_eq(S(S("login")), argv[0])) {
		if (argc < 3) {
			*this << "+BADPARAM login <user> <passphrase>\n";
			return;
		}

		account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Incorrect account name or passphrase\n";
			return;
		}

		if (!account->check_passphrase(argv[2])) {
			account = NULL;
			*this << "+NOTFOUND Incorrect account name or passphrase\n";
			return;
		}

		Log::Info << "User '" << account->get_id() << "' logged in over control interface";
		*this << "+OK Welcome " << account->get_id() << "\n";
	// unknown command
	} else {
		*this << "+BADCOMMAND " << argv[0] << "\n";
	}
}

int
SControlManager::initialize ()
{
	struct passwd* pwd;

	StringList users = explode(SettingsManager.get_control_users(), ',');
	StringList admins = explode(SettingsManager.get_control_admins(), ',');

	for (StringList::iterator i = users.begin(); i != users.end(); ++i) {
		pwd = getpwnam(*i);
		if (pwd != NULL) {
			Log::Info << "User '" << pwd->pw_name << "' is a control user";
			user_list.push_back(pwd->pw_uid);
		} else {
			Log::Error << "Unknown user '" << *i << "' given as control user";
		}
	}

	for (StringList::iterator i = admins.begin(); i != admins.end(); ++i) {
		pwd = getpwnam(*i);
		if (pwd != NULL) {
			Log::Info << "User '" << pwd->pw_name << "' is a control admin";
			admin_list.push_back(pwd->pw_uid);
		} else {
			Log::Error << "Unknown user '" << *i << "' given as control admin";
		}
	}

	return 0;
}

void
SControlManager::shutdown ()
{
}

bool
SControlManager::has_user_access (uid_t uid) const
{
	if (std::find(user_list.begin(), user_list.end(), uid) != user_list.end())
		return true;

	return has_admin_access(uid);
}

bool
SControlManager::has_admin_access (uid_t uid) const
{
	return std::find(admin_list.begin(), admin_list.end(), uid) != admin_list.end();
}
