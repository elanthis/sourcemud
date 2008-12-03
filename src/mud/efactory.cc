/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/log.h"
#include "common/string.h"
#include "mud/efactory.h"
#include "mud/entity.h"

_MEntityFactory MEntityFactory;
_MEntityFactory::FactoryList* _MEntityFactory::factories = 0;

int
_MEntityFactory::initialize ()
{
	return 0;
}

void
_MEntityFactory::shutdown ()
{
	delete factories;
}

void
_MEntityFactory::register_factory (const IEntityFactory* factory)
{
	assert(factory != NULL);

	if (factories == 0)
		factories = new FactoryList();

	factories->insert(std::pair<std::string, const IEntityFactory*>(strlower(factory->get_name()), factory));
}

Entity*
_MEntityFactory::create (const std::string& name) const
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
Entity::create (const std::string& name)
{
	return MEntityFactory.create(name);
}
