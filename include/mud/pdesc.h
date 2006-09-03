/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_PDESC_H
#define AWEMUD_MUD_PDESC_H

#include "common/string.h"
#include "mud/server.h"
#include "common/imanager.h"
#include "mud/idmap.h"

DECLARE_IDMAP(CreatureTrait)

// Trait values; not just an IDMap because we have to store the desc, too
class CreatureTraitValue {
	public:
	inline CreatureTraitValue (void) : value(0) {}
	inline CreatureTraitValue (const CreatureTraitValue& ct) : value(ct.value) {}

	inline String get_name(void) const;
	inline String get_desc(void) const;

	inline uint get_value (void) const { return value; }
	inline bool valid (void) const { return value != 0; }

	inline static CreatureTraitValue lookup (String name);

	inline operator bool (void) const { return valid(); }
	inline bool operator < (const CreatureTraitValue& tv) const { return value < tv.value; }
	inline bool operator == (const CreatureTraitValue& tv) const { return value == tv.value; }

	private:
	uint value;

	friend class SCreatureTraitManager;
};

// Loads all known traits
class SCreatureTraitManager : public IManager
{
	public:
	int initialize ();
	void shutdown ();

	String get_desc(uint id) const;
	String get_name(uint id) const;

	CreatureTraitValue get_trait(String name) const;

	private:
	StringList names;
	StringList descs;
};
extern SCreatureTraitManager CreatureTraitManager;

// Implementstions
String
CreatureTraitValue::get_name () const
{
	return CreatureTraitManager.get_name(value);
}

String
CreatureTraitValue::get_desc () const
{
	return CreatureTraitManager.get_desc(value);
}

CreatureTraitValue
CreatureTraitValue::lookup (String name)
{
	return CreatureTraitManager.get_trait(name);
}

#endif
