/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "mud/idmap.h"

MID::MID()
{
}

MID::~MID()
{
}

const std::string* MID::lookup(const std::string& idname)
{
	IDMap::iterator i = id_map.find(idname);
	if (i != id_map.end())
		return &*i;
	return NULL;
}

const std::string* MID::create(const std::string& idname)
{
	if (idname.empty())
		return NULL;
	IDMap::iterator i = id_map.find(idname);
	if (i != id_map.end())
		return &*i;
	std::pair<IDMap::iterator, bool> result = id_map.insert(idname);
	return &*result.first;
}
