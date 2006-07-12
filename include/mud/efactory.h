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

class Entity;

// Creates an entity from a DBEntry
class IEntityFactory
{
	public:
	virtual ~IEntityFactory () {}
	virtual Entity* create () = 0;
};

class SEntityFactoryManager : public IManager
{
	public:
	int initialize ();
	void shutdown ();

	void add_factory (String klass, IEntityFactory*);

	private:
	typedef GCType::map<String, IEntityFactory*> FactoryList;
	FactoryList factories;
};

extern SEntityFactoryManager EntityFactoryManager;

#define BEGIN_EFACTORY(name) \
	namespace { \
		class Factory_ ## name : public IEntityFactory, public IManager { \
			virtual int initialize () { \
				require(EntityFactoryManager); \
				EntityFactoryManager.add_factory(#name, this); \
				return 0; \
			} \
			virtual void shutdown () {} \
			virtual Entity* create () {
#define END_EFACTORY \
			} \
		} factory_ ## name; \
	}

#endif
