/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"

#include "common/types.h"
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

void NetAddr::applyMask(uint mask)
{
	// determine appropriate addr and size
#ifdef HAVE_IPV6
	uint8* addr = family == AF_INET6 ? (uint8*) & in6.sin6_addr :
	              (uint8*) & in.sin_addr;
	const size_t size = family == AF_INET6 ? sizeof(in6.sin6_addr) :
	                    sizeof(in.sin_addr);
#else
	uint8* addr = (uint8*) & in.sin_addr;
	const size_t size = sizeof(in.sin_addr);
#endif

	// ensure mask is within the limits of size
	assert(mask <= size * 8);

	// buffer to hold mask
	uint8 maskbuf[sizeof(*this)];
	memset(maskbuf, 0, size);

	// number of whole bytes
	uint q = mask >> 3;
	if (q > 0)
		memset(maskbuf, 0xff, q);

	// remainder bits
	uint r = mask & 7;
	if (r > 0)
		*(maskbuf + q) = (0xff00 >> r) & 0xff;

	// apply to addr
	for (uint i = 0; i < size; ++i)
		addr[i] &= maskbuf[i];
}
