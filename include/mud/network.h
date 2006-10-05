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
#include "common/strbuf.h"

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

	const ConnList& get_conn_list () const { return connections; }
	inline uint get_total () { return total_conns; }

	private:
	ConnList connections;
	int total_conns;
};

class ISocketHandler : public GC {
	public:
	virtual ~ISocketHandler () {}

	virtual void sock_flush () = 0;
	virtual void sock_in_ready () = 0;
	virtual void sock_out_ready () = 0;
	virtual void sock_hangup () = 0;

	virtual int sock_get_fd () = 0;
	virtual bool sock_is_out_waiting () = 0;
	virtual bool sock_is_disconnect_waiting () = 0;
	virtual void sock_complete_disconnect () = 0;
};

class SocketListener : public ISocketHandler {
	public:
	inline SocketListener (int s_sock) : sock(s_sock) {}

	// sub_classes provide sock_in_ready to accept() incoming connections

	virtual void sock_flush () {}
	virtual void sock_out_ready () {}
	virtual void sock_hangup () {}

	virtual inline int sock_get_fd () { return sock; }
	virtual bool sock_is_out_waiting () { return false; }
	virtual bool sock_is_disconnect_waiting () { return false; }
	virtual void sock_complete_disconnect () {}

	protected:
	int sock;
};

class SocketConnection : public ISocketHandler {
	public:
	SocketConnection (int s_sock);

	// called with input
	virtual void sock_input (char* buffer, size_t size) = 0;

	// sub-classes must implement:
	//  void sock_flush ()
	//  void sock_hangup ()

	// request close
	void sock_disconnect ();

	// add data to the output buffer
	void sock_buffer (const char* data, size_t size);

	// internal
	virtual void sock_in_ready ();
	virtual void sock_out_ready ();
	virtual int sock_get_fd () { return sock; }
	virtual bool sock_is_out_waiting () { return !output.empty(); }
	virtual bool sock_is_disconnect_waiting () { return disconnect; }
	virtual void sock_complete_disconnect ();

	private:
	GCType::vector<char> output;
	int sock;
	bool disconnect;
};

class SNetworkManager : public IManager {
	public:
	virtual int initialize ();
	virtual void shutdown ();

	int add_socket (ISocketHandler* socket);

	int poll (long timeout);

	inline const String& get_host () const { return host; }

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
