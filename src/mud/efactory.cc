/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <assert.h>

#include "mud/efactory.h"
#include "common/log.h"

SEntityFactoryManager EntityFactoryManager;

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
SEntityFactoryManager::add_factory (String klass, IEntityFactory* factory)
{
	assert(!klass.empty());
	assert(factory != NULL);

	factories.insert(std::pair<String, IEntityFactory*>(klass, factory));
}
