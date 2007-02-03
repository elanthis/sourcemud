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
	virtual String get_name () const = 0;
	virtual ~IEntityFactory () {}
	virtual Entity* create () const = 0;
};

class SEntityFactoryManager : public IManager
{
	public:
	int initialize ();
	void shutdown ();

	Entity* create (String name) const;

	static void register_factory (const IEntityFactory*);

	private:
	typedef GCType::map<String, const IEntityFactory*> FactoryList;
	static FactoryList* factories;
};

extern SEntityFactoryManager EntityFactoryManager;

#define BEGIN_EFACTORY(name) \
	namespace { \
		namespace _Factory##name { \
			class _Factory##name : public IEntityFactory { \
				public: \
				virtual String get_name () const { return S(#name); } \
				_Factory##name () { \
					SEntityFactoryManager::register_factory(this); \
				} \
				virtual Entity* create () const {
#define END_EFACTORY \
				} \
			} _factory; \
		} \
	}

#endif
