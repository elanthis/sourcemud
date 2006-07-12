/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_SOCIAL_H
#define AWEMUD_MUD_SOCIAL_H

#include "common/string.h"
#include "mud/fileobj.h"
#include "common/types.h"
#include "mud/server.h"
#include "common/imanager.h"

// actions performed by socials
struct SocialAction : public GC
{
	String self;
	String others;
	String target;

	int load (File::Reader& reader);
};

// socials have special adverb commands
struct SocialAdverb : public GC
{
	String name;
	SocialAction action; // normal
	SocialAction person; // blah at PERSON
	SocialAction thing;  // blah at OBJECT
	SocialAction ghost;  // normal when dead
	SocialAdverb* next;
	class Social* social; // our social

	String get_name () const { return name; }

	int load (File::Reader& reader);
};

// hold a social
class Social : public GC
{
	public:
	Social ();

	// basic info
	String get_name () const { return name; }
	const SocialAdverb* get_adverb (String name) const;
	const SocialAdverb* get_default () const { return adverbs; }

	// flags
	inline bool need_speech () const { return flags.speech; }
	inline bool need_touch () const { return flags.touch; }
	inline bool need_movement () const { return flags.move || need_touch() || need_speech(); }

	private:
	// basic info
	String name;
	SocialAdverb* adverbs;
	Social* next;

	// flags
	struct SocialFlags {
		int speech:1, move:1, touch:1;
	} flags;

	// do load
	int load (File::Reader& reader);

	// for list management...
	friend class SSocialManager;
};

class SSocialManager : public IManager
{
	public:
	inline SSocialManager() : socials(NULL) {}

	int initialize ();

	void shutdown ();

	const Social* find_social (String name);

	private:
	Social* socials;
};
extern SSocialManager SocialManager;

#endif
