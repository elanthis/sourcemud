/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef UNIQID_H
#define UNIQID_H

#include <sys/time.h>

#include "mud/server.h"
#include "common/imanager.h"
#include "common/types.h"
#include "common/string.h"

class UniqueID
{
	public:
	inline UniqueID () : random(0), usecs(0), clock(0), seq(0), secs(0) {}

	friend bool operator< (const UniqueID& u1, const UniqueID& u2);
	friend bool operator== (const UniqueID& u1, const UniqueID& u2);
	inline operator bool () const { return random || usecs || clock || seq || secs; }

	private:
	uint32 random;
	uint32 usecs:20, clock:12;
	uint32 seq;
	uint32 secs;

	friend class SUniqueIDManager;
};

class SUniqueIDManager : public IManager
{
	public:
	virtual int initialize ();
	virtual void shutdown ();
	virtual void save ();

	void create (UniqueID& uid);
	inline UniqueID create () { UniqueID uid; create(uid); return uid; }
	String encode (const UniqueID& uid);
	UniqueID decode (String string);

	private:
	struct timeval last_time;
	uint32 seq;
	uint16 clock;
};

inline bool operator< (const UniqueID& u1, const UniqueID& u2)
{
	return u1.seq < u2.seq ||
	u1.usecs < u2.usecs ||
	u1.clock < u2.clock ||
	u1.random < u2.random ||
	u1.secs < u2.secs;
}

inline bool operator== (const UniqueID& u1, const UniqueID& u2)
{
	return u1.seq == u2.seq &&
	u1.usecs == u2.usecs &&
	u1.clock == u2.clock &&
	u1.random == u2.random &&
	u1.secs == u2.secs;
}

extern SUniqueIDManager UniqueIDManager;

#endif
