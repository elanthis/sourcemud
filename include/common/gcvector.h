/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_GCVECTOR_H
#define SOURCEMUD_COMMON_GCVECTOR_H

#include "common/gcbase.h"

#include <vector>

namespace GCType
{
	// GC traced version of std::vector
	template <typename T>
	struct vector : public std::vector<T, gc_allocator<T> >, public GC
	{
		public:
		vector(void) : std::vector<T, gc_allocator<T> >() {}
		vector(size_t n) : std::vector<T, gc_allocator<T> >(n) {}
	};
}

#endif
