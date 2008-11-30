/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <cstdlib>

#include "mud/settings.h"
#include "mud/uniqid.h"
#include "common/log.h"

_MUniqueID MUniqueID;

const size_t LIMIT_AMOUNT = 1024;

int
_MUniqueID::initialize ()
{
	next = 0;
	limit = 0;

	FILE* file = fopen((MSettings.get_misc_path() + "/uniqid").c_str(), "r");
	if (file == NULL) {
		Log::Error << "MUniqueID.initialize(): Failed to open uniqid: " << strerror(errno);
		return -1;
	}

	char buffer[128];
	fgets(buffer, sizeof(buffer), file);

	fclose(file);

	next = limit = strtol(buffer, NULL, 10);

	return 0;
}

void
_MUniqueID::shutdown ()
{
	FILE* file = fopen((MSettings.get_misc_path() + "/uniqid").c_str(), "w");
	if (file == NULL) {
		Log::Error << "MUniqueID.shutdown(): Failed to open uniqid: " << strerror(errno);
		return;
	}

	StringBuffer buffer;
	buffer << next;
	fprintf(file, "%s\n", buffer.c_str());

	fclose(file);
}

int
_MUniqueID::reserve ()
{
	limit = next + LIMIT_AMOUNT;

	FILE* file = fopen((MSettings.get_misc_path() + "/uniqid").c_str(), "w");
	if (file == NULL) {
		Log::Error << "MUniqueID.reserve(): Failed to open uniqid: " << strerror(errno);
		return -1;
	}

	StringBuffer buffer;
	buffer << limit;
	fprintf(file, "%s\n", buffer.c_str());

	fclose(file);

	return 0;
}

void
_MUniqueID::save ()
{
}

UniqueID
_MUniqueID::create ()
{
	UniqueID id;

	if (next == limit)
		reserve();

	id.id = next++;
	
	return id;
}

std::string
_MUniqueID::encode (UniqueID uid)
{
	StringBuffer buffer;
	buffer << uid.id;
	return buffer.str();
}

UniqueID
_MUniqueID::decode (std::string string)
{
	UniqueID uid;
	uid.id = strtol(string.c_str(), NULL, 10);
	return uid;
}
