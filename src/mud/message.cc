/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "mud/message.h"
#include "mud/fileobj.h"
#include "mud/settings.h"
#include "common/log.h"

_MMessage MMessage;

int
_MMessage::initialize (void)
{
	File::Reader reader;

	// open messages file
	if (reader.open(MSettings.get_misc_path() + "/messages"))
		return -1;

	// read said file
	FO_READ_BEGIN
		FO_WILD("text")
			messages[node.get_name()] = node.get_string();
	FO_READ_ERROR
		// damnable errors!
		return -1;
	FO_READ_END

	// all good
	return 0;
}

void
_MMessage::shutdown (void)
{
	messages.clear();
}

std::string
_MMessage::get (std::string id)
{
	MessageMap::iterator i = messages.find(id);
	return i != messages.end() ? i->second : std::string();
}
