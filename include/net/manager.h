/*
 * Source MUD
 * Copyright(C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_NET_MANAGER_H
#define SOURCEMUD_NET_MANAGER_H

#include "common.h"
#include "common/types.h"
#include "common/imanager.h"
#include "common/strbuf.h"
#include "mud/server.h"
#include "net/socket.h"
#include "net/iplist.h"
#include "config.h"

// defaults
static const uint DEFAULT_MAX_HOST_CONNS = 10;
static const uint DEFAULT_MAX_CONNS = 1000;

enum PollHandlerFlags {
	POLLSYS_READ = 1,
	POLLSYS_WRITE = 2
};

class _MNetwork : public IManager {
	public:
	virtual int initialize();
	virtual void shutdown();

	int add_socket(class ISocketHandler* socket);

	int poll(long timeout);

	inline const std::string& get_host() const { return host; }

	// track connections
	IPConnList connections;

	// IP deny list
	IPDenyList denies;

	private:
	std::string host;

	class PollData* p_data; // private implementation data
};

extern _MNetwork MNetwork;

#endif
