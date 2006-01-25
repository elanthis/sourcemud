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

typedef int64 DBID; // SQLite3 uses a 64-bit signed int for row ids

class DBEntry {
	public:
	// entry descriptors
	inline DBID get_id () const { return id; }
	inline String get_class () const { return klass; }

	// basic property management
	bool has_property (StringArg name) const;
	String get_property (StringArg name) const;
	void set_property (StringArg name, StringArg value);
	void clear ();

	// typed property management
	bool get_property_bool (StringArg name) const;
	int32 get_property_int32 (StringArg name) const;
	int64 get_property_int64 (StringArg name) const;

	void set_property_bool (StringArg name, bool value);
	void set_property_int32 (StringArg name, int32 value);
	void set_property_int64 (StringArg name, int64 value);

	private:
	DBID id;
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
	int get_entry (DBID id, DBEntry& out);

	// stores an entry in the database; returns -1 on error
	int put_entry (const DBEntry& in);

	// allocates a new object ID
	DBID new_entry (StringArg klass);

	private:
	class DBPrivate* db;
};
extern SDBManager DBManager;

#endif
