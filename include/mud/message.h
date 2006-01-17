/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef MESSAGE_H
#define MESSAGE_H

#include "awestr.h"
#include "server.h"
#include "imanager.h"
#include "gcmap.h"

class SMessageManager : public IManager
{
	public:
	virtual int initialize (void);
	virtual void shutdown (void);

	String get (StringArg id);

	private:
	typedef GCType::map<String, String> MessageMap;
	MessageMap messages;
};
extern SMessageManager MessageManager;

#endif
