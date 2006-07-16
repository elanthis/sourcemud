/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#include "mud/settings.h"
#include "mud/uniqid.h"
#include "common/log.h"

SUniqueIDManager UniqueIDManager;

const size_t LIMIT_AMOUNT = 1024;

int
SUniqueIDManager::initialize ()
{
	next = 0;
	limit = 0;

	FILE* file = fopen(SettingsManager.get_misc_path() + "/uniqid", "r");
	if (file == NULL) {
		Log::Error << "UniqueIDManager.initialize(): Failed to open uniqid: " << strerror(errno);
		return -1;
	}

	char buffer[128];
	fgets(buffer, sizeof(buffer), file);

	fclose(file);

	next = strtol(buffer, NULL, 10);

	return reserve();
}

void
SUniqueIDManager::shutdown ()
{
	FILE* file = fopen(SettingsManager.get_misc_path() + "/uniqid", "w");
	if (file == NULL) {
		Log::Error << "UniqueIDManager.shutdown(): Failed to open uniqid: " << strerror(errno);
		return;
	}

	StringBuffer buffer;
	buffer << next;
	fprintf(file, "%s\n", buffer.c_str());

	fclose(file);
}

int
SUniqueIDManager::reserve ()
{
	limit = next + LIMIT_AMOUNT;

	FILE* file = fopen(SettingsManager.get_misc_path() + "/uniqid", "w");
	if (file == NULL) {
		Log::Error << "UniqueIDManager.reserve(): Failed to open uniqid: " << strerror(errno);
		return -1;
	}

	StringBuffer buffer;
	buffer << limit;
	fprintf(file, "%s\n", buffer.c_str());

	fclose(file);

	return 0;
}

void
SUniqueIDManager::save ()
{
}

UniqueID
SUniqueIDManager::create ()
{
	UniqueID id;

	if (next == limit)
		reserve();

	id.id = next++;
	
	return id;
}

String
SUniqueIDManager::encode (UniqueID uid)
{
	StringBuffer buffer;
	buffer << uid.id;
	return buffer.str();
}

UniqueID
SUniqueIDManager::decode (String string)
{
	UniqueID uid;
	uid.id = strtol(string.c_str(), NULL, 10);
	return uid;
}
