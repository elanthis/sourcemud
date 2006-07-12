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

SSocialManager SocialManager;

namespace {
	void command_social (Character* ch, String args[])
	{
		const Social* social = SocialManager.find_social(args[0]);
		const SocialAdverb* adverb;
		if (args[1])
			adverb = social->get_adverb(args[1]);
		else
			adverb = social->get_default();

		Entity* target = NULL;
		if (args[2]) {
			target = ch->cl_find_any(args[2], false);
			if (!target)
				return;
		}

		ch->do_social(adverb, target);
	}
}

Social::Social (void) : name(), adverbs(NULL), next(NULL)
{
	flags.speech = false;
	flags.move = false;
	flags.touch = false;
}

// find an adverb in a social
const SocialAdverb*
Social::get_adverb (String name) const
{
	// search
	for (SocialAdverb* adverb = adverbs; adverb != NULL; adverb = adverb->next) {
		if (phrase_match(adverb->name, name))
			return adverb;
	}

	// no match
	return NULL;
}

// find a social from list
const Social*
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

// load a social
int
Social::load (File::Reader& reader)
{
	SocialAdverb* last = NULL;

	FO_READ_BEGIN
		FO_ATTR("name")
			name = node.get_data();
		FO_ATTR("speech")
			flags.speech = str_is_true(node.get_data());
		FO_ATTR("move")
			flags.move = str_is_true(node.get_data());
		FO_ATTR("touch")
			flags.touch = str_is_true(node.get_data());
		FO_OBJECT("adverb")
			// allocate
			SocialAdverb* adverb = new SocialAdverb();
			if (adverb == NULL) {
				Log::Error << "Out of memory for adverb";
				return -1;
			}
			adverb->social = this;

			// do load
			if (adverb->load(reader))
				return -1;

			if (last != NULL)
				last->next = adverb;
			else
				adverbs = adverb;
			adverb->next = NULL;
			last = adverb;
	FO_READ_ERROR
		return -1;
	FO_READ_END

	if (!name)
		throw File::Error(S("Social has no name"));

	return 0;
}

// load all socials
int
SSocialManager::initialize (void)
{
	require(CommandManager);

	Log::Info << "Loading socials";

	File::Reader reader;
	String path = SettingsManager.get_misc_path() + "/socials";

	// load document
	if (reader.open(path)) {
		Log::Error << "Failed to open " << path << ": " << strerror(errno);
		return -1;
	}

	// reader loop
	FO_READ_BEGIN
		// is a social?
		FO_OBJECT("social")
			// allocate
			Social* social = new Social();
			if (social == NULL) {
				Log::Error << "Out of memory for social";
				return -1;
			}

			// do load
			if (social->load(reader))
				return -1;
			social->next = socials;
			socials = social;
	FO_READ_ERROR
		return -1;
	FO_READ_END

	// register all socials as commands
	for (Social* social = socials; social != NULL; social = social->next) {
		StringBuffer buffer;

		// make usage string
		for (SocialAdverb* adverb = social->adverbs; adverb != NULL; adverb = adverb->next) {
			if (adverb->get_name() != "default")
				buffer << social->get_name() << " [" << adverb->get_name() << "] [<target>]\n";
			else
				buffer << social->get_name() << " [<target>]\n";
		}

		// create command
		Command* cmd = new Command(social->get_name(), buffer.str(), AccessID());

		// add formats
		for (SocialAdverb* adverb = social->adverbs; adverb != NULL; adverb = adverb->next) {
			CommandFormat* fmt = new CommandFormat(cmd, 200);
			fmt->set_callback(command_social);

			// build format string
			buffer.clear();
			buffer << ":0" << social->get_name();
			if (adverb->get_name() != "default")
				buffer << " :1" << adverb->get_name();
			buffer << " :2*?";
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
SSocialManager::shutdown (void)
{
	while (socials != NULL) {
		Social* temp = socials;
		socials = socials->next;
		delete temp;
	}
}

// load action
int
SocialAction::load (File::Reader& reader)
{
	// read loop
	FO_READ_BEGIN
		FO_ATTR("you")
			self = node.get_data();
		FO_ATTR("others")
			others = node.get_data();
		FO_ATTR("target")
			target = node.get_data();
	FO_READ_ERROR
		return -1;
	FO_READ_END

	return 0;
}

// load adverb
int
SocialAdverb::load (File::Reader& reader)
{
	// reader loop
	FO_READ_BEGIN
		FO_ATTR("name")
			name = node.get_data();
		FO_OBJECT("action")
			action.load(reader);
		FO_OBJECT("person")
			person.load(reader);
		FO_OBJECT("thing")
			thing.load(reader);
		FO_OBJECT("ghost")
			ghost.load(reader);
	FO_READ_ERROR
		return -1;
	FO_READ_END

	if (!name)
		throw File::Error(S("Adverb has no name"));

	return 0;
}
