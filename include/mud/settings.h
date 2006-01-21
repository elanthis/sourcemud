/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "common/awestr.h"
#include "mud/server.h"
#include "common/imanager.h"

#define SETTING_INT(name) int val_ ## name; inline int get_ ## name (void) const { return val_ ## name; }
#define SETTING_STRING(name) String val_ ## name; inline const String& get_ ## name (void) const { return val_ ## name; }
#define SETTING_BOOL(name) bool val_ ## name; inline bool get_ ## name (void) const { return val_ ## name; }

class SSettingsManager : public IManager
{
	public:
	int initialize (void);
	void shutdown (void) {}

	void print_usage (void);
	int parse_argv (int argc, char** argc);
	int load_file (StringArg path);

	SETTING_STRING(log_file)
	SETTING_STRING(pid_file)
	SETTING_STRING(deny_file)
	SETTING_STRING(state_file)
	SETTING_STRING(config_file)
	SETTING_STRING(control_sock)
	SETTING_STRING(control_users)
	SETTING_STRING(control_admins)
	SETTING_STRING(account_path)
	SETTING_STRING(blueprint_path)
	SETTING_STRING(ai_path)
	SETTING_STRING(zone_path)
	SETTING_STRING(world_path)
	SETTING_STRING(player_path)
	SETTING_STRING(help_path)
	SETTING_STRING(scripts_path)
	SETTING_STRING(misc_path)
	SETTING_STRING(abuse_email)
	SETTING_STRING(bugs_email)
	SETTING_STRING(admin_email)
	SETTING_STRING(hostname)
	SETTING_STRING(user)
	SETTING_STRING(group)
	SETTING_STRING(chroot)
	SETTING_STRING(sendmail_bin)
	SETTING_INT(port)
	SETTING_INT(max_per_host)
	SETTING_INT(max_clients)
	SETTING_INT(chars_per_account)
	SETTING_INT(active_per_account)
	SETTING_INT(auto_save)
	SETTING_INT(activity_timeout)
	SETTING_BOOL(daemon)
	SETTING_BOOL(ipv6)
	SETTING_BOOL(account_creation)
	SETTING_BOOL(backup_players)
	SETTING_BOOL(backup_accounts)
	SETTING_BOOL(backup_zones)
};

extern SSettingsManager SettingsManager;

#undef SETTING_INT
#undef SETTING_STRING
#undef SETTING_BOOL

#endif
