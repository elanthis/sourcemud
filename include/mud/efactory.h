/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef EFACTORY_H
#define EFACTORY_H

#include "awestr.h"
#include "server.h"
#include "imanager.h"
#include "gcmap.h"

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
