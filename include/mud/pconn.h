/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_PCONN_H
#define SOURCEMUD_MUD_PCONN_H

#include "common/types.h"

class Player;

class IPlayerConnection {
	public:
	virtual ~IPlayerConnection () {}

	virtual void pconn_connect (Player* player) = 0;
	virtual void pconn_disconnect () = 0;
	virtual void pconn_write (const char* data, size_t len) = 0;
	virtual void pconn_set_echo (bool value) = 0;
	virtual void pconn_set_indent (uint level) = 0;
	virtual void pconn_set_color (int color, int value) = 0;
	virtual void pconn_clear () = 0;
	virtual void pconn_force_prompt () = 0;
	virtual uint pconn_get_width () = 0;
};

#endif
