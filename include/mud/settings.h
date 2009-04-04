/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "common/imanager.h"
#include "mud/server.h"

#define SETTING_INT(ident, name) int val_ ## name; inline int get ## ident() const { return val_ ## name; }
#define SETTING_STRING(ident, name) std::string val_ ## name; inline const std::string& get ## ident() const { return val_ ## name; }
#define SETTING_BOOL(ident, name) bool val_ ## name; inline bool get ## ident() const { return val_ ## name; }

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

	SETTING_STRING(LogFile, log_file)
	SETTING_STRING(HttpLogFile, http_log_file)
	SETTING_STRING(PidFile, pid_file)
	SETTING_STRING(DenyFile, deny_file)
	SETTING_STRING(StateFile, state_file)
	SETTING_STRING(ConfigFile, config_file)
	SETTING_STRING(AccountPath, account_path)
	SETTING_STRING(BlueprintPath, blueprint_path)
	SETTING_STRING(AiPath, ai_path)
	SETTING_STRING(ZonePath, zone_path)
	SETTING_STRING(WorldPath, world_path)
	SETTING_STRING(PlayerPath, player_path)
	SETTING_STRING(HelpPath, help_path)
	SETTING_STRING(ScriptsPath, scripts_path)
	SETTING_STRING(DbPath, db_path)
	SETTING_STRING(MiscPath, misc_path)
	SETTING_STRING(HtmlPath, html_path)
	SETTING_STRING(SkeyPath, skey_path)
	SETTING_STRING(AbuseEmail, abuse_email)
	SETTING_STRING(BugsEmail, bugs_email)
	SETTING_STRING(AdminEmail, admin_email)
	SETTING_STRING(Hostname, hostname)
	SETTING_STRING(User, user)
	SETTING_STRING(Group, group)
	SETTING_STRING(Chroot, chroot)
	SETTING_STRING(SendmailBin, sendmail_bin)
	SETTING_INT(Port, port)
	SETTING_INT(Http, http)
	SETTING_INT(MaxPerHost, max_per_host)
	SETTING_INT(MaxClients, max_clients)
	SETTING_INT(CharactersPerAccount, characters_per_account)
	SETTING_INT(ActivePerAccount, active_per_account)
	SETTING_INT(AutoSave, auto_save)
	SETTING_INT(TelnetTimeout, telnet_timeout)
	SETTING_INT(HttpTimeout, http_timeout)
	SETTING_BOOL(Daemon, daemon)
	SETTING_BOOL(Ipv6, ipv6)
	SETTING_BOOL(AccountCreation, account_creation)
	SETTING_BOOL(BackupPlayers, backup_players)
	SETTING_BOOL(BackupAccounts, backup_accounts)
	SETTING_BOOL(BackupZones, backup_zones)

private:
	std::tr1::unordered_map<std::string, SettingInfo*> by_name;
};

extern _MSettings MSettings;

#undef SETTING_INT
#undef SETTING_STRING
#undef SETTING_BOOL

#endif
