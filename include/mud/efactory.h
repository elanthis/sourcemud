/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_EFACTORY_H
#define AWEMUD_MUD_EFACTORY_H

#include "common/string.h"
#include "common/imanager.h"
#include "common/gcmap.h"
#include "common/db.h"

class Entity;

// Creates an entity from a DBEntry
class IEntityFactory
{
	public:
	virtual ~IEntityFactory () {}
	virtual Entity* create (const DBEntry& data) = 0;
};

class SEntityFactoryManager : public IManager
{
	public:
	int initialize ();
	void shutdown ();

	void add_factory (StringArg klass, IEntityFactory*);

	Entity* load (DBID id); // load the given ID
	Entity* load (DBID id, StringArg klass); // load the given ID if the class matches

	private:
	typedef GCType::map<String, IEntityFactory*> FactoryList;
	FactoryList factories;
};

extern SEntityFactoryManager EntityFactoryManager;

#endif
