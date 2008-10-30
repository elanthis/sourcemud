/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SERVER_H
#define SERVER_H

#include "common/string.h"

#define SERVER_MAX_CLIENTS 10000

namespace MUD {
	// shutdown the server
	void shutdown ();

	// get uptime
	String get_uptime ();
}

#endif
