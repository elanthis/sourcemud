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

class IPlayerConnection
{
public:
	virtual ~IPlayerConnection() {}

	virtual void pconnConnect(Player* player) = 0;
	virtual void pconnDisconnect() = 0;
	virtual void pconnWrite(const char* data, size_t len) = 0;
	virtual void pconnSetEcho(bool value) = 0;
	virtual void pconnSetIndent(uint level) = 0;
	virtual void pconnSetColor(int color, int value) = 0;
	virtual void pconnClear() = 0;
	virtual void pconnForcePrompt() = 0;
	virtual uint pconnGetWidth() = 0;
};

#endif
