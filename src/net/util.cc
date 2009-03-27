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

// compare addresses
int Network::addrcmp(const NetAddr& addr1, const NetAddr& addr2)
{
	// same family, yes?
	if (addr1.family != addr2.family)
		return addr1.family < addr2.family ? -1 : 1;

	switch (addr1.family) {
		/* IPv4 */
	case AF_INET:
		return memcmp(&addr1.in.sin_addr, &addr2.in.sin_addr,
		              sizeof(addr1.in.sin_addr));
		/* IPv6 */
#ifdef HAVE_IPV6
	case AF_INET6:
		return !IN6_ARE_ADDR_EQUAL(&sin1->sin6_addr, &sin2->sin6_addr);
#endif
		/* Unknown */
	default:
		assert(0 && "unknown address type");
		return 0;
	}
}

// compare addresses - with mask applied to *first* address (only)
int Network::addrcmpMask(const NetAddr& in_addr1, const NetAddr& addr2, uint mask)
{
	// same family, yes?
	if (in_addr1.family != addr2.family)
		return -1;

	NetAddr addr1 = in_addr1;
	addr1.applyMask(mask);

	switch (addr1.family) {
		/* IPv4 */
	case AF_INET:
		return memcmp(&addr1.in.sin_addr, &addr2.in.sin_addr, sizeof(addr1.in.sin_addr));
		/* IPv6 */
#ifdef HAVE_IPV6
	case AF_INET6:
		return !IN6_ARE_ADDR_EQUAL(&addr1.in6.sin6_addr, &addr2.in6.sin6_addr);
#endif
		/* Unknown */
	default:
		assert(0 && "unknown address type");
		return 0;
	}
}

// listen/server connection
int Network::listenTcp(int port, int family)
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
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &i_opt, sizeof(i_opt));

	// setup and config
#ifdef HAVE_IPV6
	if (family == AF_INET6) {
		// IPv6
		ss_len = sizeof(sockaddr_in6);
		memset(&ss, 0, sizeof(ss));
		ss.family = AF_INET6;
		ss.in6.sin6_port = htons(port);

#ifdef IPV6_V6ONLY
		// set IPV6-only
		i_opt = 1;
		setsockopt(sock, SOL_SOCKET, IPV6_V6ONLY, &i_opt, sizeof(i_opt));
#endif
	} else
#endif // HAVE_IPV6
		if (family == AF_INET) {
			// IPv4
			ss_len = sizeof(sockaddr_in);
			memset(&ss, 0, sizeof(ss));
			ss.family = AF_INET;
			((sockaddr_in*)&ss)->sin_port = htons(port);
		}

	// bind socket
	if (bind(sock, (struct sockaddr *)&ss, ss_len) == -1) {
		Log::Error << "bind() failed: " << strerror(errno);
		close(sock);
		return -1;;
	}

	// start listening
	if (::listen(sock, 5)) {
		Log::Error << "listen() failed: " << strerror(errno);
		close(sock);
		return -1;
	}

	return sock;
}

int Network::parseAddr(const char* item, NetAddr* host, uint* mask)
{
	char buffer[128];

	// put in buffer, snprintf() guarnatees NUL byte
	snprintf(buffer, sizeof(buffer), "%s", item);

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
		addrApplyMask((uint8*)&((sockaddr_in6*)&ss)->sin6_addr, 16, inmask);
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
			ss.applyMask(inmask);
			ss.family = AF_INET;
			*host = NetAddr(ss);
			*mask = inmask;
			return 0;
#else // HAVE_INET_PTON
		if (inetAton(buffer, &((sockaddr_in*)&ss)->sin_addr) != 0) { // match
			// check mask
			if (inmask > 32)
				return -1; // FAIL
			else if (inmask < 0)
				inmask = 32;
			ss.applyMask(inmask);
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
