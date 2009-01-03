/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef UNIQID_H
#define UNIQID_H

#include "mud/server.h"
#include "common/imanager.h"
#include "common/types.h"

typedef uint32 UniqueIDBase; // FIXME: should probably default to uin64

class UniqueID
{
public:
	UniqueID() : id(0) {}

	friend bool operator< (const UniqueID& u1, const UniqueID& u2) { return u1.id < u2.id; }
	friend bool operator== (const UniqueID& u1, const UniqueID& u2) { return u1.id == u2.id; }
	operator bool () const { return id; }

private:
	UniqueIDBase id;

	friend class _MUniqueID;
};

class _MUniqueID : public IManager
{
public:
	virtual int initialize();
	virtual void shutdown();
	virtual void save();

	UniqueID create();
	std::string encode(UniqueID uid);
	UniqueID decode(const std::string& string);
	int reserve();

private:
	UniqueIDBase next;
	UniqueIDBase limit;
};

extern _MUniqueID MUniqueID;

#endif
