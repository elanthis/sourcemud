/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_COMMON_GCVECTOR_H
#define AWEMUD_COMMON_GCVECTOR_H

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
