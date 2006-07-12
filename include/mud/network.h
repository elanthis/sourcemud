/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef NETWORK_H 
#define NETWORK_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "common/types.h"
#include "common/string.h"
#include "mud/server.h"
#include "common/imanager.h"
#include "common/gcbase.h"

#include <vector>

// sockaddr_storage workarounds
#ifdef HAVE_SOCKADDR_STORAGE
typedef struct sockaddr_storage SockStorage;
#else // HAVE_SOCKADDR_STORAGE
typedef struct sockaddr_in SockStorage;
#define ss_family sin_family
#endif // HAVE_SOCKADDR_STORAGE

// defaults
static const uint DEFAULT_MAX_HOST_CONNS = 10;
static const uint DEFAULT_MAX_CONNS = 1000;

enum PollHandlerFlags {
	POLLSYS_READ = 1,
	POLLSYS_WRITE = 2
};

class IPDenyList {
	public:
	int load (String file);
	int save (String file);

	// these return -1 on invalid input, 1 on exist errors, 0 on success
	int add (String addr);
	int remove (String addr);

	bool exists (SockStorage& addr);

	private:
	struct IPDeny {
		SockStorage addr;
		uint mask;
	};
	std::vector<IPDeny> denylist;
};

class IPConnList {
	public:
	struct IPTrack {
		SockStorage addr;
		int conns;
	};
	typedef std::vector<IPTrack> ConnList;

	// return -1 if at max total users, -2 if max per host, 0 on success
	int add (SockStorage& addr);
	void remove (SockStorage& addr);

	const ConnList& get_conn_list (void) const { return connections; }
	inline uint get_total (void) { return total_conns; }

	private:
	ConnList connections;
	int total_conns;
};

class ISocketHandler : public GC {
	public:
	virtual ~ISocketHandler (void) {}

	virtual void prepare (void) = 0;
	virtual void in_ready (void) = 0;
	virtual void out_ready (void) = 0;
	virtual void hangup (void) = 0;

	virtual int get_sock (void) = 0;
	virtual char get_poll_flags (void) = 0;
};

class SocketListener : public ISocketHandler {
	public:
	inline SocketListener (int s_sock) : sock(s_sock) {}

	// must provide in_ready to accept() incoming connections

	virtual void prepare (void) {}
	virtual void out_ready (void) {}
	virtual void hangup (void) {}

	virtual inline int get_sock (void) { return sock; }
	virtual inline char get_poll_flags (void) { return POLLSYS_READ; }

	protected:
	int sock;
};

class SocketUser : public ISocketHandler {
	public:
	inline SocketUser (int s_sock) : sock(s_sock) {}

	// called with input
	virtual void in_handle (char* buffer, size_t size) = 0;
	// must implement:
	//  void prepare (void)
	//  void out_ready (void)
	//  char get_poll_flags (void)
	//  void hangup (void)

	virtual void in_ready (void);

	virtual inline int get_sock (void) { return sock; }

	protected:
	int sock;
};

class SNetworkManager : public IManager {
	public:
	virtual int initialize (void);
	virtual void shutdown (void);

	int add_socket (ISocketHandler* socket);

	int poll (long timeout);

	inline const String& get_host (void) const { return host; }

	// track connections
	IPConnList connections;

	// IP deny list
	IPDenyList denies;

	private:
	String host;

	class PollData* p_data; // private implementation data
};

namespace Network {
	// get the printable form of an IP address
	String get_addr_name (const SockStorage& addr);

	// return 0 if the two addresses are the same
	int addrcmp (const SockStorage& addr1, const SockStorage& addr2);

	// compare addresses - with mask applied to *first* address (only)
	int addrcmp_mask (const SockStorage& addr1, const SockStorage& addr2, uint mask);

	// return the UID of the peer connection on a UNIX socket
	int get_peer_uid (int sock, uid_t& uid);

	// return true if address is local
	bool is_addr_local (const SockStorage& addr);

	// listen on TCP
	int listen_tcp (int port, int family);

	// listen on UNIX
	int listen_unix (String path);

	// get socket from a tcp listener
	int accept_tcp (int sock, SockStorage& addr);

	// get socket from a UNIX listener
	int accept_unix (int sock, uid_t& uid);
};

extern SNetworkManager NetworkManager;

#endif
