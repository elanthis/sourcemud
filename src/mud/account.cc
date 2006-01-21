/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <ctype.h>

#include "mud/account.h"
#include "mud/fileobj.h"
#include "mud/settings.h"
#include "common/md5.h"

SAccountManager AccountManager;

Account::Account (StringArg s_id) : id(s_id), active(0), maxchars(0), maxactive(0)
{
	flags.disabled = false;
}

Account::~Account (void)
{
	// remove from account list
	SAccountManager::AccountList::iterator i = find(AccountManager.accounts.begin(), AccountManager.accounts.end(), this);
	if (i != AccountManager.accounts.end())
		AccountManager.accounts.erase(i);
}

int
Account::save (void) const
{
	String path = SettingsManager.get_account_path() + "/" + strlower(id) + ".acct";
	Log::Info << "Saving account " << id;

	// open
	File::Writer writer;
	if (writer.open(path))
		return -1;

	// save it out
	writer.attr("name", name);
	writer.attr("email", email);
	writer.attr("passphrase", pass);
	for (StringList::const_iterator i = chars.begin(); i != chars.end(); ++i)
		writer.attr("character", *i);
	if (flags.disabled)
		writer.attr("disabled", "yes");
	if (maxchars > 0)
		writer.attr("maxchars", maxchars);
	if (maxactive > 0)
		writer.attr("maxactive", maxactive);
	if (timeout > 0)
		writer.attr("maxactive", timeout);
	for (AccessList::const_iterator i = access.begin(); i != access.end(); ++i)
		writer.attr("access", AccessID::nameof(*i));

	// done
	writer.close();
	return 0;
}

// password management
void
Account::set_passphrase (StringArg s_pass)
{
	// encrypt
	char enc_pass[MD5_BUFFER_SIZE];
	MD5::encrypt (s_pass.c_str(), enc_pass);

	// store
	pass = enc_pass;

	// force save
	save();
}

// check password
bool
Account::check_passphrase (StringArg s_pass) const
{
	// empty?  auto-fail
	if (!s_pass)
		return false;

	// do compare
	return MD5::compare (pass.c_str(), s_pass.c_str());
}

// add a new character
void
Account::add_character (StringArg name)
{
	// not already in list?
	if (find(chars.begin(), chars.end(), name) != chars.end())
		return;

	// ok then, add it
	chars.push_back(name);
}

// remove a character
void
Account::del_character (StringArg name)
{
	// find in list
	StringList::iterator i;
	if ((i = find(chars.begin(), chars.end(), name)) == chars.end())
		return;

	// ok then, remove it
	chars.erase(i);
}

// get max chars allowed
uint
Account::get_max_chars (void) const
{
	// explicit?
	if (maxchars > 0)
		return maxchars;

	// default
	return SettingsManager.get_chars_per_account();
}

// get max active chars allowed
uint
Account::get_max_active (void) const
{
	// explicit?
	if (maxactive > 0)
		return maxactive;

	// default
	return SettingsManager.get_active_per_account();
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
SAccountManager::initialize (void)
{

	return 0;
}

void
SAccountManager::shutdown (void)
{
	// save all accounts
	for (AccountList::iterator i = accounts.begin(); i != accounts.end(); ++i)
		(*i)->save();
}

bool
SAccountManager::valid_name (StringArg name)
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
SAccountManager::valid_passphrase (StringArg pass)
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
SAccountManager::get (StringArg in_name)
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
		FO_ATTR("name")
			account->name = node.get_data();
		FO_ATTR("email")
			account->email = node.get_data();
		FO_ATTR("passphrase")
			account->pass = node.get_data();
		FO_ATTR("character")
			account->chars.push_back(node.get_data());
		FO_ATTR("maxchars")
			FO_TYPE_ASSERT(INT);
			account->maxchars = tolong(node.get_data());
		FO_ATTR("maxactive")
			FO_TYPE_ASSERT(INT);
			account->maxactive = tolong(node.get_data());
		FO_ATTR("timeout")
			FO_TYPE_ASSERT(INT);
			account->timeout = tolong(node.get_data());
		FO_ATTR("disabled")
			FO_TYPE_ASSERT(BOOL);
			account->flags.disabled = str_is_true(node.get_data());
		FO_ATTR("access")
			account->access.insert(AccessID::create(node.get_data()));
	FO_READ_ERROR
		account = NULL;
		return NULL;
	FO_READ_END

	// add to list
	accounts.push_back(account);

	return account;
}

Account*
SAccountManager::create (StringArg name)
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
SAccountManager::exists (StringArg name)
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
