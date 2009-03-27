/*
 * Source MUD
 * Copyright(C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_NET_UTIL_H
#define SOURCEMUD_NET_UTIL_H

#include "net/netaddr.h"

namespace Network
{
	// return 0 if the two addresses are the same
	int addrcmp(const NetAddr& addr1, const NetAddr& addr2);
	int addrcmpMask(const NetAddr& in_addr1, const NetAddr& addr2, uint mask);

	// listen on TCP
	int listenTcp(int port, int family);

	// parse a network name into an address and possible mask
	int parseAddr(const char* item, NetAddr* host, uint* mask);
};

#endif
