/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <assert.h>

#include "mud/efactory.h"
#include "common/log.h"

int
SEntityFactoryManager::initialize (void)
{
	return 0;
}

void
SEntityFactoryManager::shutdown (void)
{
}

void
SEntityFactoryManager::add_factory (IEntityFactory* factory)
{
	assert(factory != NULL);

	factories[factory->get_name()] = factory;
}

Entity*
SEntityFactoryManager::create (StringArg name)
{
	FactoryList::iterator i = factories.find(name);
	if (i != factories.end())
		return i->second->create();
	else
		return NULL;
}
