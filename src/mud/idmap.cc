/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "idmap.h"
#include "log.h"

IDManager::IDManager ()
{
}

IDManager::~IDManager ()
{
}

const String*
IDManager::lookup (StringArg idname)
{
	IDMap::iterator i = id_map.find(idname);
	if (i != id_map.end())
		return &*i;
	return NULL;
}

const String*
IDManager::create (StringArg idname)
{
	if (idname.empty())
		return NULL;
	IDMap::iterator i = id_map.find(idname);
	if (i != id_map.end())
		return &*i;
	std::pair<IDMap::iterator, bool> result = id_map.insert(idname);
	return &*result.first;
}
