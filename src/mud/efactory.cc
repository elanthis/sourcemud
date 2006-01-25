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
SEntityFactoryManager::add_factory (StringArg klass, IEntityFactory* factory)
{
	assert(!klass.empty());
	assert(factory != NULL);

	factories.insert(std::pair<String, IEntityFactory*>(klass, factory));
}

Entity*
SEntityFactoryManager::load (DBID id)
{
	DBEntry entry;
	GCType::map<String, IEntityFactory*>::iterator factory;

	if (DBManager.get_entry(id, entry) != 0)
		return NULL;

	factory = factories.find(entry.get_class());
	if (factory == factories.end()) {
		Log::Error << "No entity factory found for class '" << entry.get_class() << "'";
		return NULL;
	}

	return factory->second->create(entry);
}

Entity*
SEntityFactoryManager::load (DBID id, StringArg klass)
{
	DBEntry entry;
	GCType::map<String, IEntityFactory*>::iterator factory;

	if (DBManager.get_entry(id, entry) != 0)
		return NULL;

	if (entry.get_class() != klass) {
		Log::Warning << "Entity " << entry.get_id() << " class '" << entry.get_class() << "' does not match expected class '" << klass << "'";
		return NULL;
	}

	factory = factories.find(entry.get_class());
	if (factory == factories.end()) {
		Log::Error << "No entity factory found for class '" << entry.get_class() << "'";
		return NULL;
	}

	return factory->second->create(entry);
}
