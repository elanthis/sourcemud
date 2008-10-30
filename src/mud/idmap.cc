/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "mud/idmap.h"

IDManager::IDManager ()
{
}

IDManager::~IDManager ()
{
}

const String*
IDManager::lookup (String idname)
{
	IDMap::iterator i = id_map.find(idname);
	if (i != id_map.end())
		return &*i;
	return NULL;
}

const String*
IDManager::create (String idname)
{
	if (idname.empty())
		return NULL;
	IDMap::iterator i = id_map.find(idname);
	if (i != id_map.end())
		return &*i;
	std::pair<IDMap::iterator, bool> result = id_map.insert(idname);
	return &*result.first;
}
