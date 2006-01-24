/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sqlite3.h>

#include "common/db.h"
#include "mud/settings.h"
#include "common/log.h"

struct DBPrivate {
	sqlite3* db;
	sqlite3_stmt* get_object_class;
	sqlite3_stmt* get_object_properties;
	sqlite3_stmt* create_object;
	sqlite3_stmt* put_object_properties;
};

SDBManager DBManager;

bool
DBEntry::has_property (StringArg name) const
{
	return properties.find(name) != properties.end();
}

String
DBEntry::get_property (StringArg name) const
{
	GCType::map<String,String>::const_iterator i;

	i = properties.find(name);
	if (i == properties.end())
		return String();

	return i->second;
}

void
DBEntry::set_property (StringArg name, StringArg value)
{
	properties.insert(std::pair<String,String>(name,value));
}

int
SDBManager::initialize ()
{
	Log::Info << "Initializing database";

	db = new DBPrivate();

	if (sqlite3_open(SettingsManager.get_db_path(), &db->db) != SQLITE_OK) {
		Log::Error << "sqlite3_open() failed: " << sqlite3_errmsg(db->db);
		return 1;
	}

	if (sqlite3_prepare(db->db, "SELECT class FROM objects WHERE id=?;", -1, &db->get_object_class, NULL) != SQLITE_OK) {
		sqlite3_close(db->db); // can't handle error... already handling one 
		Log::Error << "sqlite3_prepare() failed: " << sqlite3_errmsg(db->db);
		return 1;
	}

	if (sqlite3_prepare(db->db, "SELECT name,value FROM properties WHERE id=?;", -1, &db->get_object_properties, NULL) != SQLITE_OK) {
		sqlite3_close(db->db); // can't handle error... already handling one 
		Log::Error << "sqlite3_prepare() failed: " << sqlite3_errmsg(db->db);
		return 1;
	}
	
	return 0;
}

void
SDBManager::shutdown ()
{
	if (sqlite3_close(db->db) != SQLITE_OK) {
		Log::Error << "sqlite3_close() failed: " << sqlite3_errmsg(db->db);
	}
}
