/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "mud/caffect.h"

std::string CreatureAffectType::names[] = {
	S("unknown"),
	S("toxin"),
	S("disease"),
	S("innate"),
	S("magic"),
	S("psionic"),
	S("tech"),
};

CreatureAffectType
CreatureAffectType::lookup (const std::string& name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (str_eq(name, names[i]))
			return i;
	return UNKNOWN;
}

CreatureAffectGroup::CreatureAffectGroup (const std::string& s_title, CreatureAffectType s_type, uint s_duration) : title(s_title), type(s_type), duration(s_duration)
{
}

int
CreatureAffectGroup::add_affect (ICreatureAffect* affect)
{
	affects.push_back(affect);
	return 0;
}

int
CreatureAffectGroup::apply (Creature* creature) const
{
	for (AffectList::const_iterator i = affects.begin(); i != affects.end(); ++i)
		if ((*i)->apply(creature))
			return -1;
	return 0;
}

void
CreatureAffectGroup::remove (Creature* creature) const
{
	for (AffectList::const_iterator i = affects.begin(); i != affects.end(); ++i)
		(*i)->remove(creature);
}

void
CreatureAffectGroup::update (Creature* creature)
{
	if (duration > 0)
		--duration;

	for (AffectList::const_iterator i = affects.begin(); i != affects.end(); ++i)
		(*i)->update (creature);
}
