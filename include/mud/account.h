/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef __ACCOUNT_H__
#define __ACCOUNT_H__

#include "common/imanager.h"
#include "mud/server.h"
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
	Account(const std::string& s_id);
	~Account();

	// the ID
	std::string getId() const { return id; }

	// account info
	std::string getName() const { return name; }
	void setName(const std::string& s_name) { name = s_name; }

	std::string getEmail() const { return email; }
	void setEmail(const std::string& s_email) { email = s_email; }

	// pass phrases
	bool checkPassphrase(const std::string& check) const;
	void setPassphrase(const std::string& s_pass);

	// character list
	const std::vector<std::string>& getCharList() const { return characters; }
	void addCharacter(const std::string& name);
	void delCharacter(const std::string& name);

	// times
	time_t getTimeCreated() const { return time_created; }
	time_t getTimeLogin() const { return time_lastlogin; }
	void updateTimeLogin();

	// save out
	int save() const;

	// active counts
	uint getActive() { return active; }
	// manage active connections FIXME should only be available to Player...
	void incActive() { ++active; }
	void decActive() { --active; }

	// limits
	uint getMaxCharacters() const;
	uint getMaxActive() const;
	uint getTimeout() const { return timeout; }
	void setMaxCharacters(uint value) { maxcharacters = value; }
	void setMaxActive(uint value) { maxactive = value; }
	void setTimeout(uint value) { timeout = value; }
	bool isDisabled() const { return flags.disabled; }
	void setDisabled(bool value) { flags.disabled = value; }

	// access privileges
	bool hasAccess(AccessID) const;  // true if we do, false if we don't
	bool grantAccess(AccessID);  // returns true if added, false is we already have
	bool revokeAccess(AccessID);  // returns true if removed, flase if we didn't have it
	const AccessList& getAccess() const { return access; }
	bool isGM() const { return hasAccess(AccessID::lookup("gm")); }
	bool isAdmin() const { return hasAccess(AccessID::lookup("admin")); }
	bool isBuilder() const { return hasAccess(AccessID::lookup("builder")); }
	void grantAdmin() { grantAccess(AccessID::lookup("admin")); }
	void grantGm() { grantAccess(AccessID::lookup("gm")); }
	void grantBuilder() { grantAccess(AccessID::lookup("builder")); }

	// parsing
	virtual int macroProperty(const class StreamControl& stream, const std::string& method, const MacroList& argv) const;
	virtual void macroDefault(const class StreamControl& stream) const;

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
	std::vector<std::string> characters;
	AccessList access;

	struct AccountFlags {
int disabled:
		1; // no login allowed
	} flags;

	friend class _MAccount;
};

class _MAccount : public IManager
{
public:
	virtual int initialize();
	virtual void shutdown();

	std::tr1::shared_ptr<Account> get(const std::string& name);  // need a copy for get

	std::tr1::shared_ptr<Account> create(const std::string& id);  // create a new account

	bool validName(const std::string& name);  // is a name a valid account name?
	bool validPassphrase(const std::string& check);  // a valid passphrase?

	bool exists(const std::string& name);  // account already exists

private:
	typedef std::tr1::unordered_map<std::string, std::tr1::shared_ptr<Account> > AccountList;
	AccountList accounts;

	friend class Account;
};
extern _MAccount MAccount;

#endif
