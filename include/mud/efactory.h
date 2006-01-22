/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_EFACTORY_H
#define AWEMUD_MUD_EFACTORY_H

#include "common/string.h"
#include "mud/server.h"
#include "common/imanager.h"
#include "common/gcmap.h"

class Entity;

class IEntityFactory
{
	public:
	virtual ~IEntityFactory () {}
	virtual String get_name () = 0;
	virtual Entity* create () = 0;
};

class SEntityFactoryManager : public IManager
{
	public:
	int initialize ();
	void shutdown ();

	void add_factory (IEntityFactory*);

	Entity* create (StringArg name);

	private:
	typedef GCType::map<String, IEntityFactory*> FactoryList;
	FactoryList factories;
};

extern SEntityFactoryManager EntityFactoryManager;

#endif
