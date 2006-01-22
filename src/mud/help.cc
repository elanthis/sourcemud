/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <dirent.h>
#include <fnmatch.h>

#include <list>

#include "common/string.h"
#include "mud/player.h"
#include "mud/command.h"
#include "mud/settings.h"
#include "common/streams.h"
#include "mud/color.h"
#include "mud/fileobj.h"
#include "mud/parse.h"
#include "mud/help.h"

SHelpManager HelpManager;

void command_help (Player *ch, char** argv)
{
	HelpManager.print (ch, argv[0]);
}

HelpTopic*
SHelpManager::get_topic (StringArg name)
{
	for (TopicList::iterator i = topics.begin(); i != topics.end(); ++i)
		if (phrase_match((*i)->name, name))
			return *i;
	return NULL;
}

void
SHelpManager::print (Player* player, StringArg name)
{
	assert (player != NULL);

	// try a man page
	if (CommandManager.show_man(player, name, true))
		return;

	// try a help topic
	HelpTopic* topic = get_topic(name);
	if (topic) {
		*player << CSPECIAL "Help: " CNORMAL << topic->name << "\n\n";
		player->set_indent(2);
		*player << StreamParse(topic->about, "player", player) << "\n";
		player->set_indent(0);
		return;
	}

	// nope, nothin'
	*player << CSPECIAL "No help for '" << name << "' available." CNORMAL "\n";
}

int
SHelpManager::initialize (void)
{
	Log::Info << "Loading help database";

	DIR *dir;
	dirent *dent;
	String path = SettingsManager.get_help_path();

	// open directory
	dir = opendir(path);
	if (dir == NULL) {
		Log::Error << "Failed to open help folder: " << strerror(errno);
		return 1;
	}

	// read each entry
	while ((dent = readdir(dir)) != NULL) {
		// match file
		if (!fnmatch ("*.help", dent->d_name, 0)) {
			File::Reader reader;

			// open failed?
			if (reader.open(path + "/" + dent->d_name)) {
				closedir(dir);
				return -1;
			}

			// read file
			FO_READ_BEGIN
				FO_KEYED("topic")
					HelpTopic* topic = new HelpTopic();
					topic->name = node.get_key();
					topic->about = node.get_data();
					topics.push_back(topic);
			FO_READ_ERROR
				closedir(dir);
				return -1;
			FO_READ_END
		}
	}

	closedir(dir);

	return 0;
}

void
SHelpManager::shutdown (void)
{
	// delete all topics
	for (TopicList::iterator i = topics.begin(); i != topics.end(); ++i)
		delete (*i);
	topics.clear();
}
