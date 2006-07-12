/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef IDMAP_H
#define IDMAP_H

#include <algorithm>

#include "common/string.h"
#include "common/gcset.h"
#include "common/gcvector.h"

// NOTE:
// The funky static methods to retrieve the name list and intptr_t map
// are necessary but sickeningly ugly hacks to make sure that
// C++ actually initializes the containers.  Do not try to "clean
// up" or optimize the code by removing them, because it'll just
// cause some weird breakages.

class IDManager
{
	public:
	typedef GCType::set<String> IDMap;

	IDManager ();
	~IDManager ();
	
	const String* lookup (String name);
	const String* create (String name);

	const IDMap& get_all () const { return id_map; }

	private:
	IDMap id_map;
};

// must be inherited
template <typename tag>
class BaseID
{
	private:
	explicit inline BaseID (const String* s_id) : id(s_id) {}

	public:
	inline BaseID () : id(NULL) {}
	inline BaseID (const BaseID<tag>& s_id) : id(s_id.id) {}

	inline bool valid () const { return id != 0; }

	inline static BaseID<tag> lookup (String idname) { return BaseID<tag>(get_manager().lookup(idname)); }
	inline static BaseID<tag> create (String idname) { return BaseID<tag>(get_manager().create(idname)); }
	inline String name () const { return id != NULL ? *id : String(); }
	inline static String nameof (BaseID<tag> id) { return id.name(); }

	static const IDManager::IDMap& get_all () { return get_manager().get_all(); }

	inline bool operator< (const BaseID<tag>& cmp) const { return id < cmp.id; }
	inline bool operator== (const BaseID<tag>& cmp) const { return id == cmp.id; }

	private:
	const String* id;

	inline static IDManager& get_manager ();
};

template <typename tag>
IDManager&
BaseID<tag>::get_manager ()
{
	static IDManager manager;
	return manager;
}

#define DECLARE_IDMAP(name) \
	struct idmap_tag_ ## name ## _t {}; \
	typedef BaseID<idmap_tag_ ## name ## _t> name ## ID;

#endif
