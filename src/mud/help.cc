/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "common/string.h"
#include "mud/player.h"
#include "mud/command.h"
#include "mud/settings.h"
#include "common/streams.h"
#include "mud/color.h"
#include "mud/fileobj.h"
#include "mud/parse.h"
#include "mud/help.h"
#include "mud/telnet.h"
#include "common/manifest.h"

SHelpManager HelpManager;

void command_help (Player* player, String argv[])
{
	StreamControl stream(player);
	HelpManager.print (stream, argv[0]);
}

HelpTopic*
SHelpManager::get_topic (String name)
{
	for (TopicList::iterator i = topics.begin(); i != topics.end(); ++i)
		if (phrase_match((*i)->name, name))
			return *i;
	return NULL;
}

void
SHelpManager::print (StreamControl& stream, String name)
{
	// try a man page
	if (CommandManager.show_man(stream, name, true))
		return;

	// try a help topic
	HelpTopic* topic = get_topic(name);
	if (topic) {
		stream << CSPECIAL "Help: " CNORMAL << topic->name << "\n\n";
		stream << StreamIndent(2) << StreamParse(topic->about) << StreamIndent(0) << "\n";
		return;
	}

	// nope, nothin'
	stream << CSPECIAL "No help for '" << name << "' available." CNORMAL "\n";
}

int
SHelpManager::initialize ()
{
	ManifestFile man(SettingsManager.get_help_path(), S(".help"));
	StringList files = man.get_files();;
	for (StringList::iterator i = files.begin(); i != files.end(); ++i) {
		File::Reader reader;

		// open failed?
		if (reader.open(*i))
			return -1;

		// read file
		FO_READ_BEGIN
			FO_WILD("topic")
				HelpTopic* topic = new HelpTopic();
				topic->name = node.get_key();
				topic->about = node.get_data();
				topics.push_back(topic);
		FO_READ_ERROR
			return -1;
		FO_READ_END
	}

	return 0;
}

void
SHelpManager::shutdown ()
{
	// delete all topics
	for (TopicList::iterator i = topics.begin(); i != topics.end(); ++i)
		delete (*i);
	topics.clear();
}
