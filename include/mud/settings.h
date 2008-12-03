/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "common.h"
#include "common/string.h"
#include "common/imanager.h"
#include "mud/server.h"

#define SETTING_INT(name) int val_ ## name; inline int get_ ## name () const { return val_ ## name; }
#define SETTING_STRING(name) std::string val_ ## name; inline const std::string& get_ ## name () const { return val_ ## name; }
#define SETTING_BOOL(name) bool val_ ## name; inline bool get_ ## name () const { return val_ ## name; }

struct SettingInfo;

class _MSettings : public IManager
{
	public:
	int initialize();
	void shutdown() {}

	void printUsage();
	int parseArgv(int argc, char** argv);
	int loadFile(const std::string& path);

	int getInt(const std::string&);
	const std::string& getString(const std::string&);
	bool getBool(const std::string&);

	SETTING_STRING(log_file)
	SETTING_STRING(http_log_file)
	SETTING_STRING(pid_file)
	SETTING_STRING(deny_file)
	SETTING_STRING(state_file)
	SETTING_STRING(config_file)
	SETTING_STRING(account_path)
	SETTING_STRING(blueprint_path)
	SETTING_STRING(ai_path)
	SETTING_STRING(zone_path)
	SETTING_STRING(world_path)
	SETTING_STRING(player_path)
	SETTING_STRING(help_path)
	SETTING_STRING(scripts_path)
	SETTING_STRING(db_path)
	SETTING_STRING(misc_path)
	SETTING_STRING(html_path)
	SETTING_STRING(skey_path)
	SETTING_STRING(abuse_email)
	SETTING_STRING(bugs_email)
	SETTING_STRING(admin_email)
	SETTING_STRING(hostname)
	SETTING_STRING(user)
	SETTING_STRING(group)
	SETTING_STRING(chroot)
	SETTING_STRING(sendmail_bin)
	SETTING_INT(port)
	SETTING_INT(http)
	SETTING_INT(max_per_host)
	SETTING_INT(max_clients)
	SETTING_INT(characters_per_account)
	SETTING_INT(active_per_account)
	SETTING_INT(auto_save)
	SETTING_INT(telnet_timeout)
	SETTING_INT(http_timeout)
	SETTING_BOOL(daemon)
	SETTING_BOOL(ipv6)
	SETTING_BOOL(account_creation)
	SETTING_BOOL(backup_players)
	SETTING_BOOL(backup_accounts)
	SETTING_BOOL(backup_zones)

	private:
	std::map<std::string, SettingInfo*> by_name;
};

extern _MSettings MSettings;

#undef SETTING_INT
#undef SETTING_STRING
#undef SETTING_BOOL

#endif
