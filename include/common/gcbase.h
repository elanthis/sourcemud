/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_GCBASE_H
#define SOURCEMUD_COMMON_GCBASE_H

#define GC_NAME_CONFLICT 1

#include <gc/gc_cpp.h>
#include <gc/gc_allocator.h>

namespace GCType
{
	// if the object should be collectable AND NEEDS A DESTRUCTOR, use this
	class Cleanup : virtual public ::gc
	{
		public:
		Cleanup (void);
		inline virtual ~Cleanup (void) {}
		private:
		static void cleanup(void* obj, void* data);
	};

	// if the object should be collectable, will not be derived, AND NEEDS A DESTRUCTOR, use this
	class CleanupNonVirtual : virtual public ::gc
	{
		public:
		CleanupNonVirtual (void);
		private:
		static void cleanup(void* obj, void* data);
	};

	// if the object is simple (NO *IMPLICIT OR EXPLICIT* DESTRUCTOR USED)
	typedef ::gc GC;
}
using namespace GCType;

#endif
