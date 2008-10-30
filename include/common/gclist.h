/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_GCLIST_H
#define SOURCEMUD_COMMON_GCLIST_H

#include "common/gcbase.h"

#include <list>

namespace GCType
{
	// GC traced version of std::list
	template <typename T>
	struct list : public std::list<T, gc_allocator<T> >, public GC { };
}

#endif
