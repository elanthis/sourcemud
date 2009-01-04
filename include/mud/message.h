/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_MESSAGE_H
#define SOURCEMUD_MUD_MESSAGE_H

#include "common/imanager.h"
#include "mud/server.h"

class _MMessage : public IManager
{
public:
	virtual int initialize();
	virtual void shutdown();

	std::string get(const std::string& id);

private:
	typedef std::map<std::string, std::string> MessageMap;
	MessageMap messages;
};
extern _MMessage MMessage;

#endif
