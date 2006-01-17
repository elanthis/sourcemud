/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef GC_MAP_H
#define GC_MAP_H

/* fix for this file being broken... */
#ifndef AWESOMEPLAY_GC_ALLOCATOR_H
#define AWESOMEPLAY_GC_ALLOCATOR_H
#include <gc/gc_allocator.h>
#endif

#include <map>

namespace GCType
{
	// GC traced version of std::map
	template <typename K, typename T>
	struct map : public std::map<const K, T, std::less<const K>, gc_allocator<std::pair<const K, T> > >, public GC { };
	//
	// GC traced version of std::multimap
	template <typename K, typename T>
	struct multimap : public std::multimap<const K, T, std::less<const K>, gc_allocator<std::pair<const K, T> > >, public GC { };
}

#endif
