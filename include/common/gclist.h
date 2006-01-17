/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef GC_LIST_H
#define GC_LIST_H

/* fix for this file being broken... */
#ifndef AWESOMEPLAY_GC_ALLOCATOR_H
#define AWESOMEPLAY_GC_ALLOCATOR_H
#include <gc/gc_allocator.h>
#endif

#include <list>

namespace GCType
{
	// GC traced version of std::list
	template <typename T>
	struct list : public std::list<T, gc_allocator<T> >, public GC { };
}

#endif
