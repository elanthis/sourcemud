/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_COMMON_GCLIST_H
#define AWEMUD_COMMON_GCLIST_H

#include "common/gcbase.h"

#include <list>

namespace GCType
{
	// GC traced version of std::list
	template <typename T>
	struct list : public std::list<T, gc_allocator<T> >, public GC { };
}

#endif
