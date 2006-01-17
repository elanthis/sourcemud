/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef SERVER_H
#define SERVER_H

#include "awestr.h"

#define SERVER_MAX_CLIENTS 10000

namespace AweMUD {
	// shutdown the server
	void shutdown ();

	// get uptime
	String get_uptime ();
}

#endif
