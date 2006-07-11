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

SSocialManager SocialManager;

Social::Social (void) : name(), adverbs(NULL), next(NULL)
{
	flags.speech = false;
	flags.move = false;
	flags.touch = false;
}

// find an adverb in a social
const SocialAdverb*
Social::get_adverb (StringArg name) const
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
SSocialManager::find_social (StringArg name)
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
			self = node.get_data().c_str();
		FO_ATTR("others")
			others = node.get_data().c_str();
		FO_ATTR("target")
			target = node.get_data().c_str();
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
