/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/caffect.h"

String CreatureAffectType::names[] = {
	S("unknown"),
	S("toxin"),
	S("disease"),
	S("innate"),
	S("magic"),
	S("psionic"),
	S("tech"),
};

CreatureAffectType
CreatureAffectType::lookup (String name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (str_eq(name, names[i]))
			return i;
	return UNKNOWN;
}

CreatureAffectGroup::CreatureAffectGroup (String s_title, CreatureAffectType s_type, uint s_duration) : title(s_title), type(s_type), duration(s_duration)
{
}

int
CreatureAffectGroup::add_affect (ICreatureAffect* affect)
{
	affects.push_back(affect);
	return 0;
}

int
CreatureAffectGroup::apply (Creature* character) const
{
	for (AffectList::const_iterator i = affects.begin(); i != affects.end(); ++i)
		if ((*i)->apply(character))
			return -1;
	return 0;
}

void
CreatureAffectGroup::remove (Creature* character) const
{
	for (AffectList::const_iterator i = affects.begin(); i != affects.end(); ++i)
		(*i)->remove(character);
}

void
CreatureAffectGroup::update (Creature* character)
{
	if (duration > 0)
		--duration;

	for (AffectList::const_iterator i = affects.begin(); i != affects.end(); ++i)
		(*i)->update (character);
}
