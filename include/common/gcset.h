/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_GCSET_H
#define SOURCEMUD_COMMON_GCSET_H

#include "common/gcbase.h"

#include <set>

namespace GCType
{
	// GC traced version of std::set
	template <typename K>
	struct set : public std::set<K, std::less<K>, gc_allocator<K> >, public GC { };
}

#endif
