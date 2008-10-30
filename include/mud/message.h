/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_MESSAGE_H
#define SOURCEMUD_MUD_MESSAGE_H

#include "common/string.h"
#include "mud/server.h"
#include "common/imanager.h"
#include "common/gcmap.h"

class SMessageManager : public IManager
{
	public:
	virtual int initialize (void);
	virtual void shutdown (void);

	String get (String id);

	private:
	typedef GCType::map<String, String> MessageMap;
	MessageMap messages;
};
extern SMessageManager MessageManager;

#endif
