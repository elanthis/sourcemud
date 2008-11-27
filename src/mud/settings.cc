/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "common/string.h"
#include "mud/settings.h"
#include "mud/fileobj.h"
#include "common/log.h"

SSettingsManager SettingsManager;

#define SETTING_INT(name,short_opt,long_opt,file_opt,def) \
	{ #name, short_opt, long_opt, file_opt, &SettingsManager.val_ ## name, NULL, NULL, def, std::string(), false },
#define SETTING_STRING(name,short_opt,long_opt,file_opt,def) \
	{ #name, short_opt, long_opt, file_opt, NULL, &SettingsManager.val_ ## name, NULL, 0, S(def), false },
#define SETTING_BOOL(name,short_opt,long_opt,file_opt,def) \
	{ #name, short_opt, long_opt, file_opt, NULL, NULL, &SettingsManager.val_ ## name, 0, std::string(), def },

namespace {
	struct SettingInfo {
		const char* name;
		char short_opt;
		const char* long_opt;
		const char* file_opt;
		int* val_int;
		std::string* val_string;
		bool* val_bool;
		int def_int;
		std::string def_string;
		bool def_bool;
	};

	SettingInfo settings[] = {
		SETTING_STRING(log_file, 'l', "log", "log_file", "")
		SETTING_STRING(pid_file, 'p', "pid", "pid_file", "sourcemud.pid")
		SETTING_STRING(deny_file, 0, "deny", "denied_hosts_file", "")
		SETTING_STRING(account_path, 0, NULL, "account_dir", "data")
		SETTING_STRING(blueprint_path, 0, NULL, "blueprint_dir", "data/blueprints")
		SETTING_STRING(ai_path, 0, NULL, "ai_dir", "scripts/ai")
		SETTING_STRING(zone_path, 0, NULL, "zone_dir", "data/zones")
		SETTING_STRING(player_path, 0, NULL, "player_dir", "data")
		SETTING_STRING(help_path, 0, NULL, "help_dir", "data/help")
		SETTING_STRING(scripts_path, 0, NULL, "script_dir", "scripts")
		SETTING_STRING(world_path, 0, NULL, "world_dir", "data")
		SETTING_STRING(misc_path, 0, NULL, "misc_dir", "data")
		SETTING_STRING(html_path, 0, NULL, "html_dir", "data/html")
		SETTING_STRING(db_path, 'D', "db", "db", "data/sourcemud.db")
		SETTING_STRING(abuse_email, 0, NULL, "abuse_mail", "sourcemud@localhost")
		SETTING_STRING(bugs_email, 0, NULL, "bug_mail", "sourcemud@localhost")
		SETTING_STRING(admin_email, 0, NULL, "admin_mail", "sourcemud@localhost")
		SETTING_STRING(hostname, 0, NULL, "hostname", "")
		SETTING_STRING(user, 'u', "user", "user", "")
		SETTING_STRING(group, 'g', "group", "group", "")
		SETTING_STRING(chroot, 0, "chroot", "chroot", "")
		SETTING_STRING(sendmail_bin, 0, NULL, "sendmail", "")
		SETTING_STRING(config_file, 'C', "config", NULL, "")
		SETTING_STRING(state_file, 'S', "state", NULL, "state")
		SETTING_BOOL(daemon, 'd', NULL, "daemon", false)
		SETTING_BOOL(ipv6, '6', NULL, "ipv6", false)
		SETTING_BOOL(account_creation, 9, NULL, "account_creation", true)
		SETTING_BOOL(backup_players, 0, NULL, "backup_players", false)
		SETTING_BOOL(backup_accounts, 0, NULL, "backup_accounts", false)
		SETTING_BOOL(backup_zones, 0, NULL, "backup_zones", false)
		SETTING_INT(port, 'P', "port", "port", 4545)
		SETTING_INT(http, 'H', "http", "http_port", 0)
		SETTING_INT(max_per_host, 0, NULL, "max_per_host", 5)
		SETTING_INT(max_clients, 0, NULL, "max_clients", 1000)
		SETTING_INT(characters_per_account, 0, NULL, "acct_char_limit", 3)
		SETTING_INT(active_per_account, 0, NULL, "acct_play_limit", 1)
		SETTING_INT(auto_save, 0, NULL, "auto_save", 15)
		SETTING_INT(telnet_timeout, 0, NULL, "telnet_timeout", 30)
		SETTING_INT(http_timeout, 0, NULL, "http_timeout", 30)
		{ NULL, 0, NULL, NULL, NULL, NULL, NULL, 0, S(""), false }
	};
}

int
SSettingsManager::initialize (void)
{
	for (int i = 0; settings[i].name != NULL; ++i) {
		if (settings[i].val_int)
			*settings[i].val_int = settings[i].def_int;
		else if (settings[i].val_string)
			*settings[i].val_string = settings[i].def_string;
		else if (settings[i].val_bool)
			*settings[i].val_bool = settings[i].def_bool;
	}

	return 0;
}

void
SSettingsManager::print_usage (void)
{
	// FIXME - don't use fprintf
	fprintf(stderr, "./sourcemud");

	for (int i = 0; settings[i].name != NULL; ++i) {
		if (settings[i].short_opt != 0 || settings[i].long_opt != NULL) {
			fprintf(stderr, " [");
			if (settings[i].short_opt)
				fprintf(stderr, "-%c", settings[i].short_opt);
			if (settings[i].long_opt) {
				if (settings[i].short_opt)
					fprintf(stderr, "|--%s", settings[i].long_opt);
				else
					fprintf(stderr, "--%s", settings[i].long_opt);
			}
			if (settings[i].val_string)
				fprintf(stderr, " <string>]");
			else if (settings[i].val_int)
				fprintf(stderr, " <int>]");
			else
				fprintf(stderr, "]");
		}
	}

	fprintf(stderr, "\n");
}

int
SSettingsManager::parse_argv (int argc, char** argv)
{
	int err = 0;
	for (int opt = 1; opt < argc; ++opt) {
		bool found = false;
		for (int i = 0; settings[i].name != NULL; ++i) {
			if (
				(settings[i].short_opt != 0 && argv[opt][0] == '-' && argv[opt][1] == settings[i].short_opt && argv[opt][2] == 0) ||
				(settings[i].long_opt != 0 && !strncmp(argv[opt], "--", 2) && !strcmp(argv[opt] + 2, settings[i].long_opt))
			) {
				found = true;

				if (settings[i].val_string != NULL) {
					if (opt == argc - 1) {
						Log::Error << "No argument given for option: " << argv[opt];
						return -1;
					}
					*settings[i].val_string = std::string(argv[++opt]);
				} else if (settings[i].val_int != NULL) {
					if (opt == argc - 1) {
						Log::Error << "No argument given for option: " << argv[opt];
						return -1;
					}
					char* end;
					*settings[i].val_int = strtol(argv[++opt], &end, 10);
					if (*end != 0) {
						Log::Warning << "Option '" << argv[opt - 1] << "' expects integral value";
					}
				} else if (settings[i].val_bool != NULL) {
					++opt;
					if (!strcmp(argv[opt], "1") || !strcmp(argv[opt], "on") || !strcmp(argv[opt], "yes") || !strcmp(argv[opt], "true"))
						*settings[i].val_bool = true;
					else if (!strcmp(argv[opt], "0") || !strcmp(argv[opt], "off") || !strcmp(argv[opt], "no") || !strcmp(argv[opt], "false"))
						*settings[i].val_bool = false;
					else {
						Log::Warning << "Option '" << argv[opt - 1] << "' expects boolean value";
						err = -1;
					}
				}
			}
		}

		if (!found) {
			Log::Error << "Unknown option: " << argv[opt];
			err = -2;
		}
	}
	return err;
}

int
SSettingsManager::load_file (std::string path)
{
	FILE* file;
	char buffer[1024];
	char* start;
	char* sep;
	char* end;
	int line;

	Log::Info << "Reading settings from " << path;

	// open
	if ((file = fopen(path.c_str(), "r")) == NULL) {
		Log::Error << "Failed to open " << path << ": " << strerror(errno);
		return -1;
	}

	// read lines
	line = 0;
	while (fgets(buffer, sizeof(buffer), file) != NULL) {
		++ line;

		// trim beginning
		start = buffer;
		while (*start != 0 && isspace(*start))
			++start;

		// not a comment?
		if (*start == '#')
			continue;

		// trim end
		for (end = start + strlen(start) - 1; end >= start; --end)
			if (isspace(*end))
				*end = 0;
			else
				break;

		// not blank?
		if (*start == 0)
			continue;
		
		// find separator
		sep = strchr(start, '=');
		if (sep == NULL) {
			Log::Warning << "Malformed line at " << path << ':' << line;
			continue;
		}

		// trim end of key
		end = sep - 1;
		while (end >= start && isspace(*end))
			*end = 0;

		// find start of value
		++sep;
		while (*sep != 0 && isspace(*sep))
			++sep;

		// empty key or empty value?
		if (*start == 0 || *sep == 0) {
			Log::Warning << "Malformed line at " << path << ':' << line;
			continue;
		}

		// find option
		bool found = false;
		for (int i = 0; settings[i].name != NULL; ++i) {
			if (settings[i].file_opt != NULL && !strcmp(start, settings[i].file_opt)) {
				// string?
				if (settings[i].val_string != NULL) {
					*settings[i].val_string = std::string(sep);
				} else if (settings[i].val_int != NULL) {
					*settings[i].val_int = strtol(sep, &end, 10);
					if (*end != 0) {
						Log::Warning << "Option '" << settings[i].file_opt << "' expects integral value at " << path << ':' << line;
					}
				} else if (settings[i].val_bool != NULL) {
					if (!strcmp(sep, "1") || !strcmp(sep, "on") || !strcmp(sep, "yes") || !strcmp(sep, "true"))
						*settings[i].val_bool = true;
					else if (!strcmp(sep, "0") || !strcmp(sep, "off") || !strcmp(sep, "no") || !strcmp(sep, "false"))
						*settings[i].val_bool = false;
					else
						Log::Warning << "Option '" << settings[i].file_opt << "' expects boolean value at " << path << ':' << line;
				} else {
					Log::Error << "Option '" << settings[i].file_opt << "' is useless at " << path << ':' << line;
				}

				// all done here
				found = true;
				break;
			}
		}
		if (!found)
			Log::Warning << "Unknown option '" << start << "' at " << path << ':' << line;
	}

	fclose(file);

	return 0;
}
