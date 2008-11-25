/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include <ctype.h>

#include "mud/account.h"
#include "mud/fileobj.h"
#include "mud/settings.h"
#include "common/md5.h"
#include "common/time.h"

SAccountManager AccountManager;

Account::Account (String s_id) : id(s_id), active(0), maxcharacters(0), maxactive(0), timeout(0)
{
	flags.disabled = false;
	time_created = time(NULL);
	time_lastlogin = (time_t)0;
}

Account::~Account ()
{
	// remove from account list
	SAccountManager::AccountList::iterator i = find(AccountManager.accounts.begin(), AccountManager.accounts.end(), this);
	if (i != AccountManager.accounts.end())
		AccountManager.accounts.erase(i);
}

int
Account::save () const
{
	String path = SettingsManager.get_account_path() + "/" + strlower(id) + ".acct";

	// open
	File::Writer writer;
	if (writer.open(path))
		return -1;

	// save it out
	writer.attr(S("account"), S("name"), name);
	writer.attr(S("account"), S("email"), email);
	writer.attr(S("account"), S("passphrase"), pass);
	for (StringList::const_iterator i = characters.begin(); i != characters.end(); ++i)
		writer.attr(S("account"), S("character"), *i);
	if (flags.disabled)
		writer.attr(S("account"), S("disabled"), "yes");
	if (maxcharacters > 0)
		writer.attr(S("account"), S("maxcharacters"), maxcharacters);
	if (maxactive > 0)
		writer.attr(S("account"), S("maxactive"), maxactive);
	if (timeout > 0)
		writer.attr(S("account"), S("maxactive"), timeout);
	writer.attr(S("account"), S("created"), time_to_str(time_created));
	writer.attr(S("account"), S("lastlogin"), time_to_str(time_lastlogin));
	for (AccessList::const_iterator i = access.begin(); i != access.end(); ++i)
		writer.attr(S("account"), S("access"), AccessID::nameof(*i));

	// done
	writer.close();
	return 0;
}

// password management
void
Account::set_passphrase (String s_pass)
{
	// encrypt
	char enc_pass[MD5_BUFFER_SIZE];
	MD5::encrypt (s_pass.c_str(), enc_pass);

	// store
	pass = String(enc_pass);

	// force save
	save();
}

// check password
bool
Account::check_passphrase (String s_pass) const
{
	// empty?  auto-fail
	if (!s_pass)
		return false;

	// do compare
	return MD5::compare (pass.c_str(), s_pass.c_str());
}

// add a new character
void
Account::add_character (String name)
{
	// not already in list?
	if (find(characters.begin(), characters.end(), name) != characters.end())
		return;

	// ok then, add it
	characters.push_back(name);
}

// remove a character
void
Account::del_character (String name)
{
	// find in list
	StringList::iterator i;
	if ((i = find(characters.begin(), characters.end(), name)) == characters.end())
		return;

	// ok then, remove it
	characters.erase(i);
}

// get max characters allowed
uint
Account::get_max_characters () const
{
	// explicit?
	if (maxcharacters > 0)
		return maxcharacters;

	// default
	return SettingsManager.get_characters_per_account();
}

// get max active characters allowed
uint
Account::get_max_active () const
{
	// explicit?
	if (maxactive > 0)
		return maxactive;

	// default
	return SettingsManager.get_active_per_account();
}

// update login time
void
Account::update_time_lastlogin ()
{
	time_lastlogin = time(NULL);
}

// access
bool
Account::has_access(AccessID id) const
{
	return access.find(id) != access.end();
}
bool
Account::grant_access(AccessID id)
{
	// grant it
	access.insert(id);
	return true;
}
bool
Account::revoke_access(AccessID id)
{
	// find it
	AccessList::iterator i = access.find(id);
	if (i == access.end())
		return false;
	// remove it
	access.erase(i);
	return true;
}

int
Account::macro_property (const StreamControl& stream, String method, const MacroList& argv) const
{
	if (method == "id") {
		stream << id;
		return 0;
	} else if (method == "name") {
		stream << name;
		return 0;
	} else if (method == "email") {
		stream << email;
		return 0;
	} else {
		return -1;
	}
}

void
Account::macro_default (const StreamControl& stream) const
{
	stream << id;
}

int
SAccountManager::initialize ()
{

	return 0;
}

void
SAccountManager::shutdown ()
{
	// save all accounts
	for (AccountList::iterator i = accounts.begin(); i != accounts.end(); ++i)
		(*i)->save();
}

bool
SAccountManager::valid_name (String name)
{
	// length
	if (name.size() < ACCOUNT_NAME_MIN_LEN || name.size() > ACCOUNT_NAME_MAX_LEN)
		return false;

	// check characters
	for (uint i = 0; i < name.size(); ++i)
		if (!isalnum(name[i]))
			return false;

	// must be good
	return true;
}

bool
SAccountManager::valid_passphrase (String pass)
{
	// length
	if (pass.size() < ACCOUNT_PASS_MIN_LEN)
		return false;

	// must be both letters and numbers
	bool let = false;
	bool num = false;
	for (uint i = 0; i < pass.size(); ++i)
		if (isalpha(pass[i]))
			let = true;
		else if (isdigit(pass[i]))
			num = true;

	// true if both let and num are now true
	return let && num;
}

Account*
SAccountManager::get (String in_name)
{
	// force lower-case
	String name = strlower(in_name);

	// check validity
	if (!valid_name(name))
		return NULL;

	// search loaded list
	for (AccountList::iterator i = accounts.begin(); i != accounts.end(); ++i)
		if ((*i)->id == name)
			return *i;

	// try load
	File::Reader reader;

	// open
	if (reader.open(SettingsManager.get_account_path() + "/" + name + ".acct"))
		return NULL;

	// create
	Account* account = new Account(name);
	if (account == NULL)
		return NULL;

	// read it in
	FO_READ_BEGIN
		FO_ATTR("account", S("name"))
			account->name = node.get_string();
		FO_ATTR("account", S("email"))
			account->email = node.get_string();
		FO_ATTR("account", S("passphrase"))
			account->pass = node.get_string();
		FO_ATTR("account", S("character"))
			account->characters.push_back(node.get_string());
		FO_ATTR("account", S("maxcharacters"))
			account->maxcharacters = node.get_int();
		FO_ATTR("account", S("maxactive"))
			account->maxactive = node.get_int();
		FO_ATTR("account", S("timeout"))
			account->timeout = node.get_int();
		FO_ATTR("account", S("disabled"))
			account->flags.disabled = node.get_bool();
		FO_ATTR("account", S("access"))
			account->access.insert(AccessID::create(node.get_string()));
		FO_ATTR("account", "created")
			account->time_created = str_to_time(node.get_string());
		FO_ATTR("account", "lastlogin")
			account->time_lastlogin = str_to_time(node.get_string());
	FO_READ_ERROR
		account = NULL;
		return NULL;
	FO_READ_END

	// add to list
	accounts.push_back(account);

	return account;
}

Account*
SAccountManager::create (String name)
{
	// check validity
	if (!valid_name(name))
		return NULL;

	// check if account exists?
	if (get(name) != NULL)
		return NULL;

	// create
	Account* account = new Account(name);
	if (account == NULL)
		return NULL;

	// save
	account->save();

	// add to list
	accounts.push_back(account);

	return account;
}

bool
SAccountManager::exists (String name)
{
	// must be lower-case
	strlower(name);

	// must be a valid name
	if (!valid_name(name))
		return false;

	// look thru list for valid and/or connected players
	for (AccountList::iterator i = accounts.begin(); i != accounts.end(); ++i) {
		if ((*i)->get_name() == name)
			return true;
	}

	// check if player file exists
	String path = SettingsManager.get_account_path() + "/" + name + ".acct";
	struct stat st;
	int res = stat (path.c_str(), &st);
	if (res == 0)
		return true;
	if (res == -1 && errno == ENOENT)
		return false;
	Log::Error << "stat() failed for " << path << ": " << strerror(errno);	
	return true;
}
