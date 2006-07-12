/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_CAFFECT_H
#define AWEMUD_MUD_CAFFECT_H

#include "common/gcbase.h"
#include "common/string.h"
#include "common/gcvector.h"
#include "mud/char.h"

class CharacterAffectType
{
	public:
	typedef enum {
		UNKNOWN = 0,
		TOXIN,
		DISEASE,
		INNATE,
		MAGIC,
		PSIONIC,
		TECH,
		COUNT
	} type_t;
	
	public:
	inline CharacterAffectType (int s_value) : value((type_t)s_value) {}
	inline CharacterAffectType (void) : value(UNKNOWN) {}

	inline bool valid (void) const { return value != UNKNOWN; }
	inline String get_name(void) const { return names[value]; }
	inline type_t get_value (void) const { return value; }

	static CharacterAffectType lookup (String name);

	inline bool operator == (const CharacterAffectType& dir) const { return dir.value == value; }
	inline bool operator != (const CharacterAffectType& dir) const { return dir.value != value; }

	private:
	type_t value;

	static String names[];
};

class ICharacterAffect : public GC
{
	public:
	virtual int apply (Character* character) const = 0;
	virtual void remove (Character* character) const = 0;

	virtual void update (Character* character) const = 0;

	virtual ~ICharacterAffect () {}
};

class CharacterAffectGroup : public GC
{
	public:
	CharacterAffectGroup (String s_title, CharacterAffectType s_type, uint s_duration);

	int add_affect (ICharacterAffect* affect);

	int apply (Character* character) const;
	void remove (Character* character) const;

	void update (Character* character);

	inline String get_title (void) const { return title; }
	inline CharacterAffectType get_type (void) const { return type; }
	inline uint get_time_left (void) const { return duration; }

	private:
	typedef GCType::vector<ICharacterAffect*> AffectList;

	String title;
	AffectList affects;
	CharacterAffectType type;
	uint duration;
};

/* ACTUAL AFFECTS */

class CharacterAffectStat : public ICharacterAffect
{
	public:
	inline CharacterAffectStat (CharStatID s_stat, int s_mod) :
		stat(s_stat), mod(s_mod) {}

	inline virtual int apply (Character* character) const {character->set_effective_stat(stat, character->get_base_stat(stat) + mod); return 0; }
	inline virtual void remove (Character* character) const {character->set_effective_stat(stat, character->get_base_stat(stat) - mod); }

	inline virtual void update (Character* character) const {};

	private:
	CharStatID stat;
	int mod;
};

#endif
