/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/caffect.h"

String CharacterAffectType::names[] = {
	"unknown",
	"toxin",
	"disease",
	"innate",
	"magic",
	"psionic",
	"tech",
};

CharacterAffectType
CharacterAffectType::lookup (StringArg name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (str_eq(name, names[i]))
			return i;
	return UNKNOWN;
}

CharacterAffectGroup::CharacterAffectGroup (String s_title, CharacterAffectType s_type, uint s_duration) : title(s_title), type(s_type), duration(s_duration)
{
}

int
CharacterAffectGroup::add_affect (ICharacterAffect* affect)
{
	affects.push_back(affect);
	return 0;
}

int
CharacterAffectGroup::apply (Character* character) const
{
	for (AffectList::const_iterator i = affects.begin(); i != affects.end(); ++i)
		if ((*i)->apply(character))
			return -1;
	return 0;
}

void
CharacterAffectGroup::remove (Character* character) const
{
	for (AffectList::const_iterator i = affects.begin(); i != affects.end(); ++i)
		(*i)->remove(character);
}

void
CharacterAffectGroup::update (Character* character)
{
	if (duration > 0)
		--duration;

	for (AffectList::const_iterator i = affects.begin(); i != affects.end(); ++i)
		(*i)->update (character);
}
