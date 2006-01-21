/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <stdio.h>

#include <typeinfo>

#include "common/gcbase.h"

namespace GCType {
	Cleanup::Cleanup(void)
	{
		// register us if we're a GC allocated object
		void* base = GC_base((void*)this);
		if (base != NULL)
			GC_register_finalizer_no_order(base, (GC_finalization_proc)GCType::Cleanup::cleanup, (void*)((char*)this - (char*)base), NULL, NULL);
	}

	void
	Cleanup::cleanup(void* base, void* diff)
	{
		//printf("FREE: %s  (%p)\n", typeid(*(Cleanup*)((char*)base + (ptrdiff_t)diff)).name(), base);
		// call finalizer - yay finalizer...
		((Cleanup*)((char*)base + (ptrdiff_t)diff))->~Cleanup();
	}

	CleanupNonVirtual::CleanupNonVirtual(void)
	{
		// register us if we're a GC allocated object
		void* base = GC_base((void*)this);
		if (base != NULL)
			GC_register_finalizer_no_order(base, (GC_finalization_proc)GCType::CleanupNonVirtual::cleanup, (void*)((char*)this - (char*)base), NULL, NULL);
	}

	void
	CleanupNonVirtual::cleanup(void* base, void* diff)
	{
		// call finalizer - yay finalizer...
		((CleanupNonVirtual*)((char*)base + (ptrdiff_t)diff))->~CleanupNonVirtual();
	}
}
