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
	sqlite3_stmt* clear_object_properties;
	sqlite3_stmt* put_object_property;
	sqlite3_stmt* begin;
	sqlite3_stmt* commit;
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

bool
DBEntry::get_property_bool (StringArg name) const
{
	return get_property(name) == "true";
}

int32
DBEntry::get_property_int32 (StringArg name) const
{
	return tolong(get_property(name));
}

void
DBEntry::set_property_bool (StringArg name, bool value)
{
	set_property(name, value ? "true" : "false");
}

void
DBEntry::set_property_int32 (StringArg name, int32 value)
{
	set_property(name, tostr(value));
}

void
DBEntry::clear ()
{
	id = 0;
	klass.clear();
	properties.clear();
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

	if (sqlite3_prepare(db->db, "REPLACE INTO objects (id,class) VALUES(?,?);", -1, &db->create_object, NULL) != SQLITE_OK) {
		sqlite3_close(db->db); // can't handle error... already handling one 
		Log::Error << "sqlite3_prepare() failed: " << sqlite3_errmsg(db->db);
		return 1;
	}

	if (sqlite3_prepare(db->db, "DELETE FROM properties WHERE id=?;", -1, &db->clear_object_properties, NULL) != SQLITE_OK) {
		sqlite3_close(db->db); // can't handle error... already handling one 
		Log::Error << "sqlite3_prepare() failed: " << sqlite3_errmsg(db->db);
		return 1;
	}

	if (sqlite3_prepare(db->db, "INSERT INTO properties (id,name,value) VALUES(?,?,?);", -1, &db->put_object_property, NULL) != SQLITE_OK) {
		sqlite3_close(db->db); // can't handle error... already handling one 
		Log::Error << "sqlite3_prepare() failed: " << sqlite3_errmsg(db->db);
		return 1;
	}

	if (sqlite3_prepare(db->db, "BEGIN;", -1, &db->begin, NULL) != SQLITE_OK) {
		sqlite3_close(db->db); // can't handle error... already handling one 
		Log::Error << "sqlite3_prepare() failed: " << sqlite3_errmsg(db->db);
		return 1;
	}

	if (sqlite3_prepare(db->db, "COMMIT;", -1, &db->commit, NULL) != SQLITE_OK) {
		sqlite3_close(db->db); // can't handle error... already handling one 
		Log::Error << "sqlite3_prepare() failed: " << sqlite3_errmsg(db->db);
		return 1;
	}

	return 0;
}

void
SDBManager::shutdown ()
{
	if (sqlite3_finalize(db->get_object_class))
		Log::Error << "sqlite3_finalize() failed: " << sqlite3_errmsg(db->db);
	if (sqlite3_finalize(db->get_object_properties))
		Log::Error << "sqlite3_finalize() failed: " << sqlite3_errmsg(db->db);
	if (sqlite3_finalize(db->create_object))
		Log::Error << "sqlite3_finalize() failed: " << sqlite3_errmsg(db->db);
	if (sqlite3_finalize(db->clear_object_properties))
		Log::Error << "sqlite3_finalize() failed: " << sqlite3_errmsg(db->db);
	if (sqlite3_finalize(db->put_object_property))
		Log::Error << "sqlite3_finalize() failed: " << sqlite3_errmsg(db->db);
	if (sqlite3_finalize(db->begin))
		Log::Error << "sqlite3_finalize() failed: " << sqlite3_errmsg(db->db);
	if (sqlite3_finalize(db->commit))
		Log::Error << "sqlite3_finalize() failed: " << sqlite3_errmsg(db->db);
	if (sqlite3_close(db->db) != SQLITE_OK)
		Log::Error << "sqlite3_close() failed: " << sqlite3_errmsg(db->db);
}

int
SDBManager::get_entry (DBID id, DBEntry& out)
{
	out.clear();

	// read the object class
	sqlite3_bind_int(db->get_object_class, 1, id);
	if (sqlite3_step(db->get_object_class) != SQLITE_ROW)
		return -1;
	out.klass = (const char*)sqlite3_column_text(db->get_object_class, 0);
	sqlite3_reset(db->get_object_class);

	// read the object properties
	sqlite3_bind_int(db->get_object_properties, 1, id);
	while (sqlite3_step(db->get_object_properties) == SQLITE_ROW)
		out.set_property((const char*)sqlite3_column_text(db->get_object_properties, 0), (const char*)sqlite3_column_text(db->get_object_properties, 1));
	sqlite3_reset(db->get_object_properties);

	return 0;
}

int
SDBManager::put_entry (const DBEntry& in)
{
	// begin transaction
	sqlite3_step(db->begin);
	sqlite3_reset(db->begin);

	// (re)create the object
	sqlite3_bind_int(db->create_object, 1, in.get_id());
	sqlite3_bind_text(db->create_object, 2, in.get_class(), -1, SQLITE_TRANSIENT);
	sqlite3_step(db->create_object);
	sqlite3_reset(db->create_object);

	// clear any existing properties
	sqlite3_bind_int(db->clear_object_properties, 1, in.get_id());
	sqlite3_step(db->clear_object_properties);
	sqlite3_reset(db->clear_object_properties);

	// set the properties
	sqlite3_bind_int(db->put_object_property, 1, in.get_id());
	for (GCType::map<String,String>::const_iterator i = in.properties.begin(); i != in.properties.end(); ++i) {
		sqlite3_bind_text(db->put_object_property, 2, i->first, -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(db->put_object_property, 3, i->second, -1, SQLITE_TRANSIENT);
		sqlite3_step(db->put_object_property);
		sqlite3_reset(db->put_object_property);
	}
	
	// commit changes
	sqlite3_step(db->commit);
	sqlite3_reset(db->commit);

	return 0;
}

DBID
SDBManager::new_entry (StringArg klass)
{
	// create the object
	sqlite3_bind_null(db->create_object, 1);
	sqlite3_bind_text(db->create_object, 2, klass, -1, SQLITE_TRANSIENT);
	sqlite3_step(db->create_object);
	sqlite3_reset(db->create_object);

	return sqlite3_last_insert_rowid(db->db);
}
