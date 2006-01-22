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

	int load (File::Reader& reader);
};

// hold a social
class Social : public GC
{
	public:
	Social (void);

	// basic info
	StringArg get_name (void) const { return name; }
	const SocialAdverb* get_adverb (const char* name) const;
	const SocialAdverb* get_default (void) const { return adverbs; }

	// flags
	inline bool need_speech (void) const { return flags.speech; }
	inline bool need_touch (void) const { return flags.touch; }
	inline bool need_movement (void) const { return flags.move || need_touch() || need_speech(); }

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
	inline SSocialManager(void) : socials(NULL) {}

	int initialize (void);

	void shutdown (void);

	const Social* find_social (StringArg name);

	private:
	Social* socials;
};
extern SSocialManager SocialManager;

#endif
