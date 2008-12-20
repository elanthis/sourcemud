/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/log.h"
#include "net/socket.h"
#include "net/util.h"
#include "config.h"

/********************
 * Misc Helper Code *
 ********************/

// apply mask to an IP address
// math/algorithm found in FreeBSD 'route' command - thanks guys!
namespace {
	void
	addr_apply_mask (uint8* addr, uint size, uint mask)
	{
		assert(mask <= size * 8);

		// buffer to hold mask
		uint8 maskbuf[size];
		memset(maskbuf, 0, size);

		// number of whole bytes
		uint q = mask >> 3;
		if (q > 0) memset(maskbuf, 0xff, q);
		
		// remainder bytes
		uint r = mask & 7;
		if (r > 0) *(maskbuf + q) = (0xff00 >> r) & 0xff;

		// apply to addr
		for (uint i = 0; i < size; ++i) 
			addr[i] &= maskbuf[i];
	}
}

// compare addresses
int
Network::addrcmp (const NetAddr& addr1, const NetAddr& addr2)
{
	// same family, yes?
	if (addr1.family != addr2.family)
		return -1;

	switch (addr1.family) {
		/* IPv4 */
		case AF_INET: {
			const sockaddr_in* sin1 = (sockaddr_in*)&addr1;
			const sockaddr_in* sin2 = (sockaddr_in*)&addr2;

			return *(uint32*)&sin1->sin_addr != *(uint32*)&sin2->sin_addr;
		}
		/* IPv6 */
#ifdef HAVE_IPV6
		case AF_INET6: {
			const sockaddr_in6* sin1 = (sockaddr_in6*)&addr1;
			const sockaddr_in6* sin2 = (sockaddr_in6*)&addr2;

			return !IN6_ARE_ADDR_EQUAL(&sin1->sin6_addr, &sin2->sin6_addr);
		}
#endif
		/* Unknown */
		default:
			return -1;
	}
}

// compare addresses - with mask applied to *first* address (only)
int
Network::addrcmp_mask (const NetAddr& in_addr1, const NetAddr& addr2, uint mask)
{
	// same family, yes?
	if (in_addr1.family != addr2.family)
		return -1;

	NetAddr addr1 = in_addr1;

	switch (addr1.family) {
		/* IPv4 */
		case AF_INET: {
			sockaddr_in sin1 = *(sockaddr_in*)&addr1;
			const sockaddr_in sin2 = *(sockaddr_in*)&addr2;

			addr_apply_mask((uint8*)&sin1.sin_addr, sizeof(sin1.sin_addr), mask);

			return memcmp(&sin1.sin_addr, &sin2.sin_addr, sizeof(sin1.sin_addr));
		}
		/* IPv6 */
#ifdef HAVE_IPV6
		case AF_INET6: {
			sockaddr_in6 sin1 = *(sockaddr_in6*)&addr1;
			const sockaddr_in6 sin2 = *(sockaddr_in6*)&addr2;

			addr_apply_mask((uint8*)&sin1.sin6_addr, sizeof(sin1.sin6_addr), mask);

			return !IN6_ARE_ADDR_EQUAL(&sin1.sin6_addr, &sin2.sin6_addr);
		}
#endif
		/* Unknown */
		default:
			return -1;
	}
}

// listen/server connection
int
Network::listen_tcp (int port, int family)
{
	NetAddr ss;
	size_t ss_len = sizeof(ss);
	int i_opt;
	int sock;

#ifdef HAVE_IPV6
	assert(family == AF_INET6 || family == AF_INET);
#else
	assert(family == AF_INET);
#endif // HAVE_IPV6

	// create socket
	if ((sock = socket(family, SOCK_STREAM, 0)) < 0) {
		Log::Error << "socket() failed: " << strerror(errno);
		return errno;
	}

	// set reuse timeout thingy
	i_opt = 1;
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &i_opt, sizeof (i_opt));

	// setup and config
#ifdef HAVE_IPV6
	if (family == AF_INET6) {
		// IPv6
		ss_len = sizeof(sockaddr_in6);
		memset(&ss, 0, sizeof(ss));
		ss.family = AF_INET6;
		ss.in6.sin6_port = htons (port);

#ifdef IPV6_V6ONLY
		// set IPV6-only
		i_opt = 1;
		setsockopt (sock, SOL_SOCKET, IPV6_V6ONLY, &i_opt, sizeof (i_opt));
#endif
	} else
#endif // HAVE_IPV6
	if (family == AF_INET) {
		// IPv4
		ss_len = sizeof(sockaddr_in);
		memset(&ss, 0, sizeof(ss));
		ss.family = AF_INET;
		((sockaddr_in*)&ss)->sin_port = htons (port);
	}

	// bind socket
	if (bind (sock, (struct sockaddr *)&ss, ss_len) == -1) {
		Log::Error << "bind() failed: " << strerror(errno);
		close(sock);
		return -1;;
	}

	// start listening
	if (::listen (sock, 5)) {
		Log::Error << "listen() failed: " << strerror(errno);
		close(sock);
		return -1;
	}

	return sock;
}

// accept incoming connections
int
Network::accept_tcp (int sock, NetAddr& addr)
{
	// accept socket
	socklen_t sslen = sizeof(addr);
	int client = accept(sock, (struct sockaddr*)&addr, &sslen);
	if (client == -1)
		return -1;

	// set non-blocking flag
	fcntl(client, F_SETFL, O_NONBLOCK);
	return client;
}

int Network::parse_addr(const char* item, NetAddr* host, uint* mask)
{
	char buffer[128];

	// put in buffer, snprintf() guarnatees NUL byte
	snprintf (buffer, sizeof(buffer), "%s", item);

	// get mask - have we a mask?
	int inmask = -1;
	char* slash = strchr(buffer, '/');
	if (slash != NULL) {
		*slash = '\0';
		++ slash;
		// don't use atoi or strtol, guarantee we parse it right
		inmask = 0;
		while (*slash != '\0') {
			if (!isdigit(*slash))
				break;
			inmask *= 10;
			inmask += *slash - '0';
			++ slash;
		}
		// only numbers, rights?
		if (*slash != '\0')
			return -1; // FAIL
	}

	// parse address
	NetAddr ss;
#ifdef HAVE_IPV6
	// try IPv6 first
	if (inet_pton(AF_INET6, buffer, &((sockaddr_in6*)&ss)->sin6_addr) > 0) { // match
		// check mask
		if (inmask > 128)
			return -1; // FAIL
		else if (inmask < 0)
			inmask = 128;
		addr_apply_mask((uint8*)&((sockaddr_in6*)&ss)->sin6_addr, 16, inmask);
		ss.family = AF_INET6;
		*host = NetAddr(ss);
		*mask = inmask;
		return 0;
	} else
#endif // HAVE_IPV6
	// try IPv4 parsing
#ifdef HAVE_INET_PTON
	if (inet_pton(AF_INET, buffer, &((sockaddr_in*)&ss)->sin_addr) > 0) { // match
		// check mask
		if (inmask > 32)
			return -1; // FAIL
		else if (inmask < 0)
			inmask = 32;
		addr_apply_mask((uint8*)&((sockaddr_in*)&ss)->sin_addr, 4, inmask);
		ss.family = AF_INET;
		*host = NetAddr(ss);
		*mask = inmask;
		return 0;
#else // HAVE_INET_PTON
	if (inet_aton(buffer, &((sockaddr_in*)&ss)->sin_addr) != 0) { // match
		// check mask
		if (inmask > 32)
			return -1; // FAIL
		else if (inmask < 0)
			inmask = 32;
		addr_apply_mask((uint8*)&((sockaddr_in*)&ss)->sin_addr, 4, inmask);
		ss.family = AF_INET;
		*host = NetAddr(ss);
		*mask = inmask;
		return 0;
#endif
	} else {
		// no match
		return -1; // FAIL
	}
}
