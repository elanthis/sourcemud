/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_HELP_H
#define SOURCEMUD_MUD_HELP_H

#include "common/imanager.h"
#include "common/streams.h"

struct HelpTopic {
	std::string name;
	std::string about;
};

class _MHelp : public IManager
{
public:
	// startup the manager
	virtual int initialize();

	// close the manager
	virtual void shutdown();

	// print out help to a player
	void print(StreamControl& stream, const std::string& section);

	// get a topic; category is optional, its use limits
	// the topic search to topics in the given category
	HelpTopic* getTopic(const std::string& name);

private:
	typedef std::vector<HelpTopic*> TopicList;
	TopicList topics;
};
extern _MHelp MHelp;

#endif
