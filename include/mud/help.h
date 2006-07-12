/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_HELP_H
#define AWEMUD_MUD_HELP_H

#include "common/imanager.h"
#include "common/streams.h"

struct HelpTopic : public GC {
	String name;
	String about;
};

class SHelpManager : public IManager
{
	public:
	// startup the manager
	virtual int initialize (void);

	// close the manager
	virtual void shutdown (void);

	// print out help to a player
	void print (StreamControl& stream, String section);

	// get a topic; category is optional, its use limits
	// the topic search to topics in the given category
	HelpTopic* get_topic (String name);

	private:
	typedef GCType::vector<HelpTopic*> TopicList;
	TopicList topics;
};
extern SHelpManager HelpManager;

#endif
