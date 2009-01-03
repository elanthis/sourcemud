/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/file.h"
#include "common/string.h"
#include "mud/player.h"
#include "mud/account.h"
#include "mud/settings.h"

// helper function to generate path names
std::string
_MPlayer::path(const std::string& name)
{
	return MSettings.get_player_path() + "/" + strlower(name) + ".ply";
}

// check if a name is valid
bool
_MPlayer::valid_name(const std::string& name)
{
	// empty?  just fail
	if (name.empty())
		return false;

	// check size
	int len = name.size();
	if (len < PLAYER_NAME_MIN_LEN || len > PLAYER_NAME_MAX_LEN)
		return false;

	// alpha only
	for (int i = 0; i < len; i ++)
		if (!isalpha(name[i]))
			return false;

	// check 'badnames' file
	std::string path = MSettings.get_player_path() + "/badnames";

	std::ifstream badnames(path.c_str());
	if (!badnames) {
		Log::Error << "Failed to open " << path;
		return true;
	}

	char t_name[512];
	while ((badnames.getline(t_name, sizeof(t_name))))
		if (!fnmatch(t_name, name.c_str(), FNM_CASEFOLD))
			return false;

	return true;
}

// find a Player
Player *
_MPlayer::get(const std::string& name)
{
	assert(!name.empty() && "name must not be empty");

	// try loading alive
	for (PlayerList::iterator i = player_list.begin(); i != player_list.end(); ++i)
		if (((*i)->is_active()) && str_eq(name, (*i)->get_id()))
			return *i;

	// not found
	return NULL;
}

void
_MPlayer::list(const StreamControl& stream)
{
	stream << "Currently logged in players:\n";
	size_t count = 0;
	for (PlayerList::iterator i = player_list.begin(); i != player_list.end(); ++i) {
		if ((*i)->is_active()) {
			++ count;
			stream << "  " << StreamName(*i);
			if ((*i)->get_account()->isAdmin())
				stream << CADMIN " (Admin)" CNORMAL;
			else if ((*i)->get_account()->isBuilder())
				stream << CSPECIAL " (Builder)" CNORMAL;
			else if ((*i)->get_account()->isGM())
				stream << CSPECIAL " (GM)" CNORMAL;
			stream << "\n";
		}
	}
	if (count == 1)
		stream << "There is 1 player on-line.\n";
	else
		stream << "There are " << count << " players on-line.\n";
}

size_t
_MPlayer::count(void)
{
	size_t count = 0;
	for (PlayerList::iterator i = player_list.begin(); i != player_list.end(); ++i)
		if ((*i)->is_active())
			++ count;
	return count;
}

int
_MPlayer::initialize(void)
{
	// modules we need to operate
	if (require(MAccount) != 0)
		return 1;
	if (require(MEntity) != 0)
		return 1;

	return 0;
}

void
_MPlayer::shutdown(void)
{
	// quit all players
	while (!player_list.empty()) {
		player_list.erase(player_list.begin());
	}
	player_list.resize(0);
}

Player* _MPlayer::load(std::tr1::shared_ptr<Account> account, const std::string& name)
{
	// must be valid before attempting load
	if (!valid_name(name))
		return NULL;

	// already oepn?  just return
	Player* player = get(name);
	if (player != NULL)
		return player;

	// open reader
	File::Reader reader;
	if (reader.open(MPlayer.path(name)))
		return NULL;

	// create player
	player = new Player(account, name);
	if (player == NULL) {
		Log::Error << "new Player() failed";
		return NULL;
	}

	// do load
	if (player->load(reader))
		return NULL;

	// done
	return player;
}

bool
_MPlayer::exists(const std::string& name)
{
	// must be a valid name
	if (!valid_name(name))
		return false;

	// look thru list for valid and/or connected players
	for (PlayerList::iterator i = player_list.begin(); i != player_list.end(); ++i) {
		if (str_eq((*i)->get_id(), name)) {
			if ((*i)->is_connected())
				return true;
			break;
		}
	}

	// check if player file exists
	std::string path = MPlayer.path(name);
	struct stat st;
	int res = stat(path.c_str(), &st);
	if (res == 0)
		return true;
	if (res == -1 && errno == ENOENT)
		return false;
	Log::Error << "stat() failed for " << path << ": " << strerror(errno);
	return true;
}

int
_MPlayer::destroy(const std::string& name)
{
	// must be a valid name
	if (!valid_name(name))
		return 1;

	// player already on?  force quit, invalidate
	for (PlayerList::iterator i = player_list.begin(); i != player_list.end(); ++i) {
		if ((*i)->get_id() == name) {
			if ((*i)->is_connected())
				(*i)->end_session();
			break;
		}
	}

	// backup file
	std::string path = MPlayer.path(name);
	struct stat st;
	if (!stat(path.c_str(), &st)) {
		if (File::rename(path, path + "~")) { // move file
			Log::Error << "Backup of " << path << " failed: " << strerror(errno);
			return 2;
		}
	}

	Log::Info << "Player '" << name << "' has been deleted";

	// done
	return 0;
}

void
_MPlayer::save(void)
{
	for (PlayerList::iterator i = player_list.begin(); i != player_list.end(); ++i)
		if ((*i)->is_active())
			(*i)->save();
}
