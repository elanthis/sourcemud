/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef IMANAGER_H
#define IMANAGER_H

#include "awestr.h"
#include "gcvector.h"

// all *Manager classes should derive from this...
class IManager
{
	public:
	IManager ();
	inline virtual ~IManager () {}

	// return non-zero to indicate fatal error
	virtual int initialize () = 0;

	// do any necessary cleanup (save files, mostly)
	virtual void shutdown () = 0;

	// save game state
	virtual void save () {}

	// require another manager
	static int require (IManager& manager);

	// initialize all managers
	static int initialize_all ();

	// stop all managers
	static void shutdown_all ();

	// tell all managers to save state
	static void save_all ();

	private:
	static GCType::vector<IManager*>* managers;
	static GCType::vector<IManager*>* pending;
};

#endif
