/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/message.h"
#include "mud/fileobj.h"
#include "mud/settings.h"
#include "common/log.h"

SMessageManager MessageManager;

int
SMessageManager::initialize (void)
{
	File::Reader reader;

	Log::Info << "Loading messages";

	// open messages file
	if (reader.open(SettingsManager.get_misc_path() + "/messages"))
		return -1;

	// read said file
	FO_READ_BEGIN
		FO_KEYED("text")
			messages[node.get_key()] = node.get_data();
	FO_READ_ERROR
		// damnable errors!
		return -1;
	FO_READ_END

	// all good
	return 0;
}

void
SMessageManager::shutdown (void)
{
	messages.clear();
}

String
SMessageManager::get (StringArg id)
{
	MessageMap::iterator i = messages.find(id);
	return i != messages.end() ? i->second : String();
}
