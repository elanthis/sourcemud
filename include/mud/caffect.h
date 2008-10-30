/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_CAFFECT_H
#define SOURCEMUD_MUD_CAFFECT_H

#include "common/gcbase.h"
#include "common/string.h"
#include "common/gcvector.h"
#include "mud/creature.h"

class CreatureAffectType
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
	inline CreatureAffectType (int s_value) : value((type_t)s_value) {}
	inline CreatureAffectType (void) : value(UNKNOWN) {}

	inline bool valid (void) const { return value != UNKNOWN; }
	inline String get_name(void) const { return names[value]; }
	inline type_t get_value (void) const { return value; }

	static CreatureAffectType lookup (String name);

	inline bool operator == (const CreatureAffectType& dir) const { return dir.value == value; }
	inline bool operator != (const CreatureAffectType& dir) const { return dir.value != value; }

	private:
	type_t value;

	static String names[];
};

class ICreatureAffect : public GC
{
	public:
	virtual int apply (Creature* character) const = 0;
	virtual void remove (Creature* character) const = 0;

	virtual void update (Creature* character) const = 0;

	virtual ~ICreatureAffect () {}
};

class CreatureAffectGroup : public GC
{
	public:
	CreatureAffectGroup (String s_title, CreatureAffectType s_type, uint s_duration);

	int add_affect (ICreatureAffect* affect);

	int apply (Creature* character) const;
	void remove (Creature* character) const;

	void update (Creature* character);

	inline String get_title (void) const { return title; }
	inline CreatureAffectType get_type (void) const { return type; }
	inline uint get_time_left (void) const { return duration; }

	private:
	typedef GCType::vector<ICreatureAffect*> AffectList;

	String title;
	AffectList affects;
	CreatureAffectType type;
	uint duration;
};

/* ACTUAL AFFECTS */

class CreatureAffectStat : public ICreatureAffect
{
	public:
	inline CreatureAffectStat (CreatureStatID s_stat, int s_mod) :
		stat(s_stat), mod(s_mod) {}

	inline virtual int apply (Creature* character) const {character->set_effective_stat(stat, character->get_base_stat(stat) + mod); return 0; }
	inline virtual void remove (Creature* character) const {character->set_effective_stat(stat, character->get_base_stat(stat) - mod); }

	inline virtual void update (Creature* character) const {};

	private:
	CreatureStatID stat;
	int mod;
};

#endif
