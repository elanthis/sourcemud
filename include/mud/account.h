/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef __ACCOUNT_H__
#define __ACCOUNT_H__

#include <time.h>

#include <set>
#include "common/string.h"
#include "mud/server.h"
#include "common/imanager.h"
#include "mud/idmap.h"
#include "mud/macro.h"

// account name length requirements
#define ACCOUNT_NAME_MIN_LEN 3
#define ACCOUNT_NAME_MAX_LEN 15

// password limitations
#define ACCOUNT_PASS_MIN_LEN 6

DECLARE_IDMAP(Access)
typedef std::set<AccessID> AccessList;

class Account : public IMacroObject
{
	public:
	// the ID
	std::string get_id () const { return id; }

	// account info
	std::string get_name () const { return name; }
	void set_name (std::string s_name) { name = s_name; }

	std::string get_email () const { return email; }
	void set_email (std::string s_email) { email = s_email; }

	// pass phrases
	bool check_passphrase (std::string check) const;
	void set_passphrase (std::string s_pass);

	// character list
	const StringList& get_char_list () const { return characters; }
	void add_character (std::string name);
	void del_character (std::string name);

	// times
	time_t get_time_created () const { return time_created; }
	time_t get_time_lastlogin () const { return time_lastlogin; }
	void update_time_lastlogin ();

	// save out
	int save () const;

	// active counts
	uint get_active () { return active; }
	// manage active connections FIXME should only be available to Player...
	void inc_active () { ++active; }
	void dec_active () { --active; }

	// limits
	uint get_max_characters () const;
	uint get_max_active () const;
	uint get_timeout () const { return timeout; }
	void set_max_characters (uint value) { maxcharacters = value; }
	void set_max_active (uint value) { maxactive = value; }
	void set_timeout (uint value) { timeout = value; }
	bool is_disabled () const { return flags.disabled; }
	void set_disabled (bool value) { flags.disabled = value; }

	// access privileges
	bool has_access (AccessID) const; // true if we do, false if we don't
	bool grant_access (AccessID); // returns true if added, false is we already have
	bool revoke_access (AccessID); // returns true if removed, flase if we didn't have it
	const AccessList& get_access () const { return access; }
	bool is_gm () const { return has_access(AccessID::lookup(S("gm"))); }
	bool is_admin () const { return has_access(AccessID::lookup(S("admin"))); }
	bool is_builder () const { return has_access(AccessID::lookup(S("builder"))); }
	void grant_admin () { grant_access(AccessID::lookup(S("admin"))); }
	void grant_gm () { grant_access(AccessID::lookup(S("gm"))); }
	void grant_builder () { grant_access(AccessID::lookup(S("builder"))); }

	// parsing
	virtual int macro_property (const class StreamControl& stream, std::string method, const MacroList& argv) const;
	virtual void macro_default (const class StreamControl& stream) const;

	private:
	std::string id;
	std::string name;
	std::string email;
	std::string pass;
	uint active;
	uint maxcharacters; // 0 means default
	uint maxactive; // 0 means default
	uint timeout; // 0 means default
	time_t time_created;
	time_t time_lastlogin;
	StringList characters;
	AccessList access;

	struct AccountFlags {
		int disabled:1; // no login allowed
	} flags;

	Account (std::string s_id);
	~Account ();

	friend class _MAccount;
};

class _MAccount : public IManager
{
	public:
	virtual int initialize ();
	virtual void shutdown ();

	Account* get (std::string name); // need a copy for get

	Account* create (std::string id); // create a new account

	bool valid_name (std::string name); // is a name a valid account name?
	bool valid_passphrase (std::string check); // a valid passphrase?

	bool exists (std::string name); // account already exists

	private:
	typedef std::vector<Account*> AccountList;
	AccountList accounts;

	friend class Account;
};
extern _MAccount MAccount;

#endif
