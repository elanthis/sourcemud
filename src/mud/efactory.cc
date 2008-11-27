/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include <assert.h>

#include "mud/efactory.h"
#include "common/log.h"
#include "mud/entity.h"

SEntityFactoryManager EntityFactoryManager;
SEntityFactoryManager::FactoryList* SEntityFactoryManager::factories = 0;

int
SEntityFactoryManager::initialize ()
{
	return 0;
}

void
SEntityFactoryManager::shutdown ()
{
}

void
SEntityFactoryManager::register_factory (const IEntityFactory* factory)
{
	assert(factory != NULL);

	if (factories == 0)
		factories = new FactoryList();

	factories->insert(std::pair<std::string, const IEntityFactory*>(strlower(factory->get_name()), factory));
}

Entity*
SEntityFactoryManager::create (std::string name) const
{
	assert(factories != NULL);

	// find factory
	FactoryList::const_iterator i = factories->find(name);
	if (i == factories->end()) {
		Log::Warning << "Factory not found: " << name;
		return NULL;
	}

	// invoke
	return i->second->create();
}

Entity*
Entity::create (std::string name)
{
	return EntityFactoryManager.create(name);
}
