/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common/log.h"
#include "mud/settings.h"
#include "net/socket.h"
#include "net/iplist.h"
#include "net/util.h"

int IPDenyList::load(const std::string& path)
{
	char line[512];
	FILE* file;
	uint lcnt;
	
	file = fopen(path.c_str(), "r");
	if (file == NULL) {
		Log::Error << "fopen() failed: " << path << ": " << strerror(errno);
		return -1;
	}

	lcnt = 0;
	while (fgets(line, sizeof(line), file) != NULL) {
		++lcnt;

		// chew off end of line
		for (char* c = line + strlen(line) - 1; c >= line; --c)
			if (isspace(*c))
				*c = 0;
			else
				break;

		// empty? skip
		if (!strlen(line))
			continue;

		// add and handle return codes
		int err = add(std::string(line));
		if (err == -1)
			Log::Warning << "Invalid IP deny line at " << path << ":" << lcnt;
	}

	fclose(file);
	return 0;
}

int IPDenyList::save(const std::string& path)
{
	FILE* file;

	file = fopen(path.c_str(), "w");
	if (file == NULL) {
		Log::Error << "fopen() failed: " << path << ": " << strerror(errno);
		return -1;
	}

	for (std::vector<IPDeny>::iterator i = denylist.begin(); i != denylist.end(); ++i)
		fprintf(file, "%s/%u\n", Network::get_addr_name(i->addr).c_str(), i->mask);

	fclose(file);

	return 0;
}

int IPDenyList::remove(const std::string& line)
{
	SockStorage addr;
	uint mask;

	if (Network::parse_addr(line.c_str(), &addr, &mask))
		return -1;

	for (std::vector<IPDeny>::iterator i = denylist.begin(); i != denylist.end(); ++i)
		if (i->mask == mask && Network::addrcmp(addr, i->addr)) {
			denylist.erase(i);
			return 0;
		}

	return 1;
}

int IPDenyList::add(const std::string& line)
{
	SockStorage addr;
	uint mask;

	if (Network::parse_addr(line.c_str(), &addr, &mask))
		return -1;

	for (uint i = 0; i < denylist.size(); ++i)
		if (Network::addrcmp(addr, denylist[i].addr))
			return 1;

	IPDeny deny;
	deny.addr = addr;
	deny.mask = mask;
	denylist.push_back(deny);
	return 0;
}

bool
IPDenyList::exists (SockStorage& addr)
{
	for (uint i = 0; i < denylist.size(); ++i)
		if (Network::addrcmp_mask(addr, denylist[i].addr, denylist[i].mask))
			return true;
	return false;
}

/**********************
 * Connection Tracker *
 **********************/

int
IPConnList::add (SockStorage& addr)
{
	// NOTE: we never limit connections from localhost

	// too many total connections?
	if (!Network::is_addr_local(addr) && total_conns >= MSettings.get_max_clients())
		return -1;

	// find existing connection from host
	for (std::vector<IPTrack>::iterator i = connections.begin(); i != connections.end(); ++i) {
		if (!Network::addrcmp(addr, i->addr)) {
			// too many from this host?
			if (!Network::is_addr_local(addr) && i->conns >= MSettings.get_max_per_host())
				return -2;

			// inc, and return OK
			++i->conns;
			return 0;
		}
	}

	// add new host
	IPTrack track;
	track.addr = addr;
	track.conns = 1;
	connections.push_back(track);
	return 0;
}

void
IPConnList::remove (SockStorage& addr)
{
	// find existing connection from host
	for (std::vector<IPTrack>::iterator i = connections.begin(); i != connections.end(); ++i) {
		if (!Network::addrcmp(addr, i->addr)) {
			// decrement
			--i->conns;
			--total_conns;

			// at zero?  remove
			if (i->conns == 0)
				connections.erase(i);

			return;
		}
	}
}
