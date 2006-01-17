/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef GC_SET_H
#define GC_SET_H

/* fix for this file being broken... */
#ifndef AWESOMEPLAY_GC_ALLOCATOR_H
#define AWESOMEPLAY_GC_ALLOCATOR_H
#include <gc/gc_allocator.h>
#endif

#include "common/gcbase.h"
#include <set>

namespace GCType
{
	// GC traced version of std::set
	template <typename K>
	struct set : public std::set<K, std::less<K>, gc_allocator<K> >, public GC { };
}

#endif
