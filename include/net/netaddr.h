/*
 * Source MUD
 * Copyright(C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_NET_NETADDR_H 
#define SOURCEMUD_NET_NETADDR_H

#include "common.h"
#include "config.h"

struct NetAddr {
	union {
		unsigned short int family;
		struct sockaddr_in in;
#ifdef HAVE_IPV6
		struct sockaddr_in6 in6;
#endif
	};

	std::string getString(bool port = true) const;
	bool isLocal() const;
};

#endif
