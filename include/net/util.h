/*
 * Source MUD
 * Copyright(C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_NET_UTIL_H
#define SOURCEMUD_NET_UTIL_H

#include "net/socket.h"

namespace Network {
	// get the printable form of an IP address
	std::string get_addr_name(const SockStorage& addr);

	// return 0 if the two addresses are the same
	int addrcmp(const SockStorage& addr1, const SockStorage& addr2);

	// compare addresses - with mask applied to *first* address(only)
	int addrcmp_mask(const SockStorage& addr1, const SockStorage& addr2, uint mask);

	// return the UID of the peer connection on a UNIX socket
	int get_peer_uid(int sock, uid_t& uid);

	// return true if address is local
	bool is_addr_local(const SockStorage& addr);

	// listen on TCP
	int listen_tcp(int port, int family);

	// listen on UNIX
	int listen_unix(std::string path);

	// get socket from a tcp listener
	int accept_tcp(int sock, SockStorage& addr);

	// get socket from a UNIX listener
	int accept_unix(int sock, uid_t& uid);

	// parse a network name into an address and possible mask
	int parse_addr(const char* item, SockStorage* host, uint* mask);
};

#endif
