/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_COMMON_GCSET_H
#define AWEMUD_COMMON_GCSET_H

#include "common/gcbase.h"

#include <set>

namespace GCType
{
	// GC traced version of std::set
	template <typename K>
	struct set : public std::set<K, std::less<K>, gc_allocator<K> >, public GC { };
}

#endif
