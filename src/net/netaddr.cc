/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "config.h"

#include "net/netaddr.h"

std::string NetAddr::getString(bool show_port) const
{
	char hostbuf[512];
	char servbuf[512];
	char buffer[512];

	// get the info
#if HAVE_GETNAMEINFO
	getnameinfo((sockaddr*)this, sizeof(*this), hostbuf, sizeof(hostbuf), servbuf, sizeof(servbuf), NI_NUMERICHOST | NI_NUMERICSERV);
#elif defined(HAVE_INET_PTON)
	inet_ntop(AF_INET, &in.sin_addr, hostbuf, sizeof(hostbuf));
	snprintf(servbuf, sizeof(servbuf), "%d", ntohs(in.sin_port));
#else
	return "<unknown>";
#endif

	// no port?  just return the address
	if (!strlen(servbuf))
		return std::string(hostbuf);

	// display the port, and optionally port
	if (show_port) {
		if (strchr(hostbuf, ':'))
			snprintf(buffer, sizeof(buffer), "%s.%s", hostbuf, servbuf);
		else
			snprintf(buffer, sizeof(buffer), "%s:%s", hostbuf, servbuf);
	} else {
		snprintf(buffer, sizeof(buffer), "%s", hostbuf);
	}

	return std::string(buffer);
}

bool NetAddr::isLocal() const
{
	if (family == AF_INET) {
		if (in.sin_addr.s_addr == htonl(INADDR_LOOPBACK))
			return true;
	}
#ifdef HAVE_IPV6
	else if (family == AF_INET6) {
		if (IN6_IS_ADDR_LOOPBACK(&in6.sin6_addr))
			return true;
	}
#endif // HAVE_IPV6

	return false;
}
