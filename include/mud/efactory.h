/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_EFACTORY_H
#define SOURCEMUD_MUD_EFACTORY_H

#include "common/string.h"
#include "common/imanager.h"
#include <map>

class Entity;

// Creates an entity from a DBEntry
class IEntityFactory
{
	public:
	virtual const char* get_name() const = 0;
	virtual ~IEntityFactory() {}
	virtual Entity* create() const = 0;
};

class _MEntityFactory : public IManager
{
	public:
	int initialize ();
	void shutdown ();

	Entity* create (const std::string& name) const;

	static void register_factory (const IEntityFactory*);

	private:
	typedef std::map<std::string, const IEntityFactory*> FactoryList;
	static FactoryList* factories;
};

extern _MEntityFactory MEntityFactory;

#define BEGIN_EFACTORY(name) \
	namespace { \
		namespace _Factory##name { \
			class _Factory##name : public IEntityFactory { \
				public: \
				virtual const char* get_name() const { return #name; } \
				_Factory##name () { \
					_MEntityFactory::register_factory(this); \
				} \
				virtual Entity* create() const {
#define END_EFACTORY \
				} \
			} _factory; \
		} \
	}

#endif
