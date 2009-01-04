/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/imanager.h"

std::vector<IManager*>* IManager::managers = NULL;
std::vector<IManager*>* IManager::pending = NULL;

// add manager to registry
IManager::IManager()
{
	// make managers vector
	if (!pending)
		pending = new std::vector<IManager*>;

	// register
	pending->push_back(this);
}

// initialize a manager if not already handled
int IManager::require(IManager& manager)
{
	// make managers vector
	if (!managers)
		managers = new std::vector<IManager*>;

	// already initialized?
	if (std::find(managers->begin(), managers->end(), &manager) != managers->end())
		return 0;

	// try initialization
	if (manager.initialize())
		return 1;

	// add to pending list
	managers->push_back(&manager);

	// all good!
	return 0;
}

// load all managers
int IManager::initialize_all()
{
	// load manager
	for (std::vector<IManager*>::iterator i = pending->begin(); i != pending->end(); ++i)
		if (IManager::require(**i))
			return -1;

	// release pending list
	delete pending;
	pending = NULL;

	return 0;
}

// shutdown all managers
void IManager::shutdown_all()
{
	for (std::vector<IManager*>::reverse_iterator i = managers->rbegin(); i != managers->rend(); ++i)
		(*i)->shutdown();
}

// save state for all managers
void IManager::save_all()
{
	for (std::vector<IManager*>::reverse_iterator i = managers->rbegin(); i != managers->rend(); ++i)
		(*i)->save();
}

