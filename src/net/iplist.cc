/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
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

		// remove comment bits
		char* c = strchr(line, '#');
		if (c != 0)
			*c = 0;

		// chew off end of line
		for (char* e = line + strlen(line) - 1; e >= line; --e)
			if (isspace(*e))
				*e = 0;
			else
				break;

		// empty? skip
		if (line[0] == 0)
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
		fprintf(file, "%s/%u\n", i->addr.getString().c_str(), i->mask);

	fclose(file);

	return 0;
}

int IPDenyList::remove(const std::string& line)
{
	NetAddr addr;
	uint mask;

	if (Network::parseAddr(line.c_str(), &addr, &mask))
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
	NetAddr addr;
	uint mask;

	if (Network::parseAddr(line.c_str(), &addr, &mask))
		return -1;

	for (uint i = 0; i < denylist.size(); ++i)
		if (Network::addrcmp(addr, denylist[i].addr) == 0)
			return 1;

	IPDeny deny;
	deny.addr = addr;
	deny.mask = mask;
	denylist.push_back(deny);
	return 0;
}

bool IPDenyList::exists(NetAddr& addr)
{
	for (uint i = 0; i < denylist.size(); ++i)
		if (Network::addrcmpMask(addr, denylist[i].addr, denylist[i].mask) == 0)
			return true;
	return false;
}

/**********************
 * Connection Tracker *
 **********************/

int IPConnList::add(NetAddr& addr)
{
	// NOTE: we never limit connections from localhost

	// too many total connections?
	if (!addr.isLocal() && total_conns >= MSettings.getMaxClients())
		return -1;

	// find existing connection from host
	for (std::vector<IPTrack>::iterator i = connections.begin(); i != connections.end(); ++i) {
		if (!Network::addrcmp(addr, i->addr)) {
			// too many from this host?
			if (!addr.isLocal() && i->conns >= MSettings.getMaxPerHost())
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

void IPConnList::remove(NetAddr& addr)
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
