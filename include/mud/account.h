/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef __ACCOUNT_H__
#define __ACCOUNT_H__

#include "common/gcbase.h"
#include "common/gcset.h"
#include "common/awestr.h"
#include "mud/server.h"
#include "common/imanager.h"
#include "mud/idmap.h"

// account name length requirements
#define ACCOUNT_NAME_MIN_LEN 3
#define ACCOUNT_NAME_MAX_LEN 15

// password limitations
#define ACCOUNT_PASS_MIN_LEN 6

DECLARE_IDMAP(Access)
typedef GCType::set<AccessID> AccessList;

class Account : public Cleanup
{
	public:
	// the ID
	inline StringArg get_id (void) const { return id; }

	// account info
	inline StringArg get_name (void) const { return name; }
	inline void set_name (StringArg s_name) { name = s_name; }

	inline StringArg get_email (void) const { return email; }
	inline void set_email (StringArg s_email) { email = s_email; }

	// pass phrases
	bool check_passphrase (StringArg check) const;
	void set_passphrase (StringArg s_pass);

	// character list
	inline const StringList& get_char_list (void) const { return chars; }
	void add_character (StringArg name);
	void del_character (StringArg name);

	// save out
	int save (void) const;

	// active counts
	inline uint get_active (void) { return active; }
	// manage active connections FIXME should only be available to Player...
	inline void inc_active (void) { ++active; }
	inline void dec_active (void) { --active; }

	// limits
	uint get_max_chars (void) const;
	uint get_max_active (void) const;
	inline uint get_timeout (void) const { return timeout; }
	inline void set_max_chars (uint value) { maxchars = value; }
	inline void set_max_active (uint value) { maxactive = value; }
	inline void set_timeout (uint value) { timeout = value; }
	inline bool is_disabled (void) const { return flags.disabled; }
	inline void set_disabled (bool value) { flags.disabled = value; }

	// access privileges
	bool has_access (AccessID) const; // true if we do, false if we don't
	bool grant_access (AccessID); // returns true if added, false is we already have
	bool revoke_access (AccessID); // returns true if removed, flase if we didn't have it
	inline const AccessList& get_access (void) const { return access; }
	inline bool is_gm (void) const { return has_access(AccessID::lookup("gm")); }
	inline bool is_admin (void) const { return has_access(AccessID::lookup("admin")); }
	inline bool is_builder (void) const { return has_access(AccessID::lookup("builder")); }
	inline void grant_admin (void) { grant_access(AccessID::lookup("admin")); }
	inline void grant_gm (void) { grant_access(AccessID::lookup("gm")); }
	inline void grant_builder (void) { grant_access(AccessID::lookup("builder")); }

	private:
	String id;
	String name;
	String email;
	String pass;
	uint active;
	uint maxchars; // 0 means default
	uint maxactive; // 0 means default
	uint timeout; // 0 means default
	StringList chars;
	AccessList access;

	struct AccountFlags {
		int disabled:1; // no login allowed
	} flags;

	Account (StringArg s_id);
	~Account (void);

	friend class SAccountManager;
};

class SAccountManager : public IManager
{
	public:
	virtual int initialize (void);
	virtual void shutdown (void);

	Account* get (StringArg name); // need a copy for get

	Account* create (StringArg id); // create a new account

	bool valid_name (StringArg name); // is a name a valid account name?
	bool valid_passphrase (StringArg check); // a valid passphrase?

	bool exists (StringArg name); // account already exists

	private:
	typedef std::vector<Account*> AccountList;  // NOTE: do not GC-store
	AccountList accounts;

	friend class Account;
};
extern SAccountManager AccountManager;

#endif
