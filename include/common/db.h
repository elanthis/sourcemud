/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_COMMON_DB_H
#define AWEMUD_COMMON_DB_H

#include "common/gcmap.h"
#include "common/string.h"
#include "common/types.h"
#include "common/imanager.h"

class DBEntry {
	public:
	inline uint32 get_id () const { return id; }
	inline String get_class () const { return klass; }

	bool has_property (StringArg name) const;
	String get_property (StringArg name) const;
	void set_property (StringArg name, StringArg value);

	void clear ();

	private:
	uint32 id;
	String klass;
	GCType::map<String,String> properties;

	friend class SDBManager;
};

class SDBManager : public IManager
{
	public:
	virtual int initialize ();
	virtual void shutdown ();

	// get an entry from the database; returns -1 on error (not found)
	int get_entry (uint32 id, DBEntry& out);

	// stores an entry in the database; returns -1 on error
	int put_entry (const DBEntry& in);

	private:
	class DBPrivate* db;
};
extern SDBManager DBManager;

#endif
