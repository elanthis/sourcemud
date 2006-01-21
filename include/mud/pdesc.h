/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_PDESC_H
#define AWEMUD_MUD_PDESC_H

#include "common/awestr.h"
#include "mud/server.h"
#include "common/imanager.h"
#include "mud/idmap.h"

DECLARE_IDMAP(CharacterTrait)

// Trait values; not just an IDMap because we have to store the desc, too
class CharacterTraitValue {
	public:
	inline CharacterTraitValue (void) : value(0) {}
	inline CharacterTraitValue (const CharacterTraitValue& ct) : value(ct.value) {}

	inline String get_name(void) const;
	inline String get_desc(void) const;

	inline uint get_value (void) const { return value; }
	inline bool valid (void) const { return value != 0; }

	inline static CharacterTraitValue lookup (StringArg name);

	inline operator bool (void) const { return valid(); }
	inline bool operator < (const CharacterTraitValue& tv) const { return value < tv.value; }
	inline bool operator == (const CharacterTraitValue& tv) const { return value == tv.value; }

	private:
	uint value;

	friend class SCharacterTraitManager;
};

// Specific traits.. temporary hack
typedef CharacterTraitValue ColorType;
typedef CharacterTraitValue HairStyleType;
typedef CharacterTraitValue BodyBuildType;

// Loads all known traits
class SCharacterTraitManager : public IManager
{
	public:
	int initialize ();
	void shutdown ();

	String get_desc(uint id) const;
	String get_name(uint id) const;

	CharacterTraitValue get_trait(StringArg name) const;

	private:
	StringList names;
	StringList descs;
};
extern SCharacterTraitManager CharacterTraitManager;

// Implementstions
String
CharacterTraitValue::get_name () const
{
	return CharacterTraitManager.get_name(value);
}

String
CharacterTraitValue::get_desc () const
{
	return CharacterTraitManager.get_desc(value);
}

CharacterTraitValue
CharacterTraitValue::lookup (StringArg name)
{
	return CharacterTraitManager.get_trait(name);
}

#endif
