/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <list>

#include "common/string.h"
#include "mud/social.h"
#include "mud/settings.h"
#include "common/log.h"
#include "mud/command.h"
#include "mud/char.h"
#include "mud/room.h"
#include "mud/filetab.h"
#include "mud/object.h"

SSocialManager SocialManager;

namespace {
	void command_social (Character* ch, String args[])
	{
		const Social* social = SocialManager.find_social(args[0]);

		Entity* target = NULL;
		if (args[2]) {
			target = ch->cl_find_any(args[2], false);
			if (!target)
				return;
		}

		ch->do_social(social, target, args[1]);
	}
}

Social::Social () : name(), details(), next(NULL)
{
}

SocialDetails::SocialDetails ()
{
	flags.speech = false;
	flags.move = false;
	flags.touch = false;
	flags.target = false;
}

// find a social from list
Social*
SSocialManager::find_social (String name)
{
	assert (name);

	// search
	for (Social* social = socials; social != NULL; social = social->next)
		if (phrase_match(social->get_name(), name))
			return social;

	// no match
	return NULL;
}

// load all socials
int
SSocialManager::initialize ()
{
	require(CommandManager);

	File::TabReader reader;
	String path = SettingsManager.get_misc_path() + "/socials";

	// load document
	if (reader.open(path)) {
		Log::Error << "Failed to open " << path << ": " << strerror(errno);
		return -1;
	}
	if (reader.load()) {
		Log::Error << "Failed to parse " << path << ": " << strerror(errno);
		return -1;
	}

	// loop over entries
	for (size_t i = 0; i < reader.size(); ++i) {
		String name = reader.get(i, 0);
		if (name.empty()) {
			Log::Error << "Social has empty name at " << path << ":" << reader.get_line(i);
			continue;
		}

		SocialDetails detail;
		detail.adverb = reader.get(i, 1);
		StringList flags = explode(reader.get(i, 2), '|');
		detail.self = reader.get(i, 3);
		detail.room = reader.get(i, 4);
		detail.target = reader.get(i, 5);

		if (detail.self.empty()) {
			Log::Error << "Social has empty self text at " << path << ":" << reader.get_line(i);
			continue;
		}
		if (detail.room.empty()) {
			Log::Error << "Social has empty self text at " << path << ":" << reader.get_line(i);
			continue;
		}

		if (!detail.target.empty())
			detail.flags.target = true;

		for (StringList::iterator f = flags.begin(); f != flags.end(); ++f) {
			if (*f == "speech")
				detail.flags.speech = true;
			else if (*f == "move")
				detail.flags.move = true;
			else if (*f == "touch") {
				detail.flags.move = true;
				detail.flags.touch = true;
			} else
				Log::Warning << "Social has unknown flag '" << *f << "' at " << path << ":" << reader.get_line(i);
		}

		Social* social = find_social(name);
		if (!social) {
			social = new Social();
			social->name = name;
			social->next = socials;
			socials = social;
		}

		social->details.push_back(detail);
	}

	// register all socials as commands
	for (Social* social = socials; social != NULL; social = social->next) {
		StringBuffer buffer;

		// make usage string
		for (GCType::vector<SocialDetails>::iterator i = social->details.begin(); i != social->details.end(); ++i) {
			if (i->adverb.empty()) {
				if (i->flags.target)
					buffer << social->get_name() << " <target>\n";
				else
					buffer << social->get_name() << "\n";
			} else {
				if (i->flags.target)
					buffer << social->get_name() << " " << i->adverb << " <target>\n";
				else
					buffer << social->get_name() << " " << i->adverb << "\n";
			}
		}

		// create command
		Command* cmd = new Command(social->get_name(), buffer.str(), AccessID());

		// add formats
		for (GCType::vector<SocialDetails>::iterator i = social->details.begin(); i != social->details.end(); ++i) {
			CommandFormat* fmt = new CommandFormat(cmd, 200);
			fmt->set_callback(command_social);

			// build format string
			buffer.clear();
			buffer << ":0" << social->get_name();
			if (!i->adverb.empty())
				buffer << " :1" << i->adverb;
			if (i->flags.target)
				buffer << " :2*";
			if (fmt->build(buffer.str())) {
				Log::Error << "Failed to compile social command format: " << buffer.str();
				delete cmd;
				delete fmt;
				return -1;
			}
			cmd->add_format(fmt);
		}

		// all done
		CommandManager.add(cmd);
	}

	return 0;
}

void
SSocialManager::shutdown ()
{
	while (socials != NULL) {
		Social* temp = socials;
		socials = socials->next;
		delete temp;
	}
}

int
Social::perform (Character* actor, Entity* target, String adverb) const
{
	for (GCType::vector<SocialDetails>::const_iterator i = details.begin(); i != details.end(); ++i) {
		// find a match?  has right target parameters?
		if (adverb == i->adverb && ((target == NULL && !i->flags.target) || (target != NULL && i->flags.target))) {
			// check if we can perform this action
			if (i->flags.speech && !actor->can_talk()) {
				*actor << "You cannot talk.\n";
				return 1;
			}
			if (i->flags.move && !actor->can_act()) {
				*actor << "You cannot move.\n";
				return 1;
			}
			if (i->flags.touch && !actor->can_act() || (OBJECT(target) && !OBJECT(target)->is_touchable())) {
				*actor << "You cannot touch " << StreamName(target, DEFINITE, false) << ".\n";
				return 1;
			}

			// do the action
			*actor << StreamParse(i->self, S("actor"), actor, S("target"), target, S("room"), actor->get_room()) << "\n";
			if (CHARACTER(target))
				*CHARACTER(target) << StreamParse(i->self, S("actor"), actor, S("target"), target, S("room"), actor->get_room()) << "\n";
			*actor->get_room() << StreamIgnore(actor) << StreamIgnore(CHARACTER(target)) << StreamParse(i->self, S("actor"), actor, S("target"), target, S("room"), actor->get_room()) << "\n";
			return 0;
		}
	}

	// no match; guaranteed to be because of presence/lack-of target */
	if (target == NULL)
		*actor << "You must supply a target to do that.\n";
	else
		*actor << "You cannot do that to " << StreamName(target, DEFINITE, false) << ".\n";
	return -1;
}
