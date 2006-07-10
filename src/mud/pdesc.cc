/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "common/gcmap.h"
#include "mud/pdesc.h"
#include "common/string.h"
#include "mud/settings.h"
#include "mud/fileobj.h"

SCharacterTraitManager CharacterTraitManager;

String
SCharacterTraitManager::get_name (uint id) const
{
	// check that id is valid; no assert, as this is not an error
	if (id == 0 || id > names.size())
		return String();

	// return the name
	return names[id - 1];
}

String
SCharacterTraitManager::get_desc (uint id) const
{
	// check that id is valid; no assert, as this is not an error
	if (id == 0 || id > descs.size())
		return String();

	// return the description
	return descs[id - 1];
}

CharacterTraitValue
SCharacterTraitManager::get_trait (StringArg name) const
{
	CharacterTraitValue ret;
	// search traits
	for (uint i = 0; i < names.size(); ++i)
		if (str_eq(name, names[i])) {
			ret.value = i + 1;
			break;
		}
	return ret;
}

int
SCharacterTraitManager::initialize ()
{
	// open the file
	String path = SettingsManager.get_misc_path() + "/traits";
	File::Reader reader;
	if (reader.open(path))
		return -1;

	FO_READ_BEGIN
		// this is one of the available traits
		FO_ATTR("trait")
			FO_TYPE_ASSERT(STRING)
			CharacterTraitID::create(node.get_data());
		// this is one of the available trait values
		FO_KEYED("value")
			FO_TYPE_ASSERT(STRING)
			names.push_back(node.get_key());
			descs.push_back(node.get_data());
	FO_READ_ERROR
		return -1;
	FO_READ_END
	return 0;
}

void
SCharacterTraitManager::shutdown ()
{
}
