/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/string.h"
#include "common/error.h"
#include "common/file.h"
#include "common/streams.h"
#include "mud/hooks.h"
#include "mud/object.h"
#include "mud/server.h"
#include "mud/settings.h"

_MObjectBP MObjectBP;

ObjectBP::ObjectBP()
{
	weight = 0;
	cost = 0;
}

bool ObjectBP::setName(const std::string& s_name)
{
	bool ret = name.setFull(s_name);
	return ret;
}

EntityName ObjectBP::getName() const
{
	return name;
}

bool ObjectBP::hasTag(TagID tag) const
{
	return tags.find(tag) != tags.end();
}

int ObjectBP::addTag(TagID tag)
{
	tags.insert(tag);
	return 0;
}

int ObjectBP::removeTag(TagID tag)
{
	// find
	TagList::iterator ti = std::find(tags.begin(), tags.end(), tag);
	if (ti == tags.end())
		return 1;

	// remove
	tags.erase(ti);

	return 0;
}

void ObjectBP::save(File::Writer& writer)
{
	if (!isAnonymous())
		writer.attr("blueprint", "id", id);

	writer.attr("blueprint", "name", name.getFull());
	writer.attr("blueprint", "desc", desc);

	for (std::vector<std::string>::const_iterator i = keywords.begin(); i != keywords.end(); ++i)
		writer.attr("blueprint", "keyword", *i);

	writer.attr("blueprint", "equip", equip.getName());

	writer.attr("blueprint", "cost", cost);
	writer.attr("blueprint", "weight", weight);

	writer.attr("blueprint", "hidden", flags.test(ObjectFlag::HIDDEN));
	writer.attr("blueprint", "gettable", flags.test(ObjectFlag::GET));
	writer.attr("blueprint", "touchable", flags.test(ObjectFlag::TOUCH));
	writer.attr("blueprint", "dropable", flags.test(ObjectFlag::DROP));
	writer.attr("blueprint", "trashable", flags.test(ObjectFlag::TRASH));
	writer.attr("blueprint", "rotting", flags.test(ObjectFlag::ROT));

	if (locations.test(ObjectLocation::IN))
		writer.attr("blueprint", "container", "in");
	if (locations.test(ObjectLocation::ON))
		writer.attr("blueprint", "container", "on");

	for (TagList::iterator i = tags.begin(); i != tags.end(); ++i)
		writer.attr("blueprint", "tag", i->name());

	// script hook
	Hooks::saveObjectBlueprint(this, writer);
}

int ObjectBP::load(File::Reader& reader)
{
	FO_READ_BEGIN
	FO_ATTR("blueprint", "id")
	id = node.getString();
	FO_ATTR("blueprint", "name")
	setName(node.getString());
	FO_ATTR("blueprint", "keyword")
	keywords.push_back(node.getString());
	FO_ATTR("blueprint", "desc")
	setDesc(node.getString());
	FO_ATTR("blueprint", "weight")
	setWeight(node.getInt());
	FO_ATTR("blueprint", "cost")
	setCost(node.getInt());
	FO_ATTR("blueprint", "equip")
	setEquip(EquipSlot::lookup(node.getString()));
	FO_ATTR("blueprint", "gettable")
	setFlag(ObjectFlag::GET, node.getBool());
	FO_ATTR("blueprint", "touchable")
	setFlag(ObjectFlag::TOUCH, node.getBool());
	FO_ATTR("blueprint", "hidden")
	setFlag(ObjectFlag::HIDDEN, node.getBool());
	FO_ATTR("blueprint", "dropable")
	setFlag(ObjectFlag::DROP, node.getBool());
	FO_ATTR("blueprint", "trashable")
	setFlag(ObjectFlag::TRASH, node.getBool());
	FO_ATTR("blueprint", "rotting")
	setFlag(ObjectFlag::ROT, node.getBool());
	FO_ATTR("blueprint", "container")
	if (node.getString() == "on") {
		locations.set(ObjectLocation::ON);
	} else if (node.getString() == "in") {
		locations.set(ObjectLocation::IN);
	} else
		Log::Warning << "Unknown container type '" << node.getString() << "' at " << reader.getFilename() << ':' << node.getLine();
	FO_ATTR("blueprint", "tag")
	tags.insert(TagID::create(node.getString()));
	FO_READ_ERROR
	return -1;
	FO_READ_END

	return 0;
}

int _MObjectBP::initialize()
{
	// requirements
	if (require(MEvent) != 0)
		return 1;

	std::vector<std::string> files = File::dirlist(MSettings.getBlueprintPath());
	File::filter(files, "*.objs");
	for (std::vector<std::string>::iterator i = files.begin(); i != files.end(); ++i) {
		// load from file
		File::Reader reader;
		if (reader.open(*i))
			return -1;
		FO_READ_BEGIN
		FO_OBJECT("blueprint", "object")
		ObjectBP* blueprint = new ObjectBP();
		if (blueprint->load(reader)) {
			Log::Warning << "Failed to load blueprint in " << reader.getFilename() << " at " << node.getLine();
			return -1;
		}

		if (blueprint->getId().empty()) {
			Log::Warning << "Blueprint has no ID in " << reader.getFilename() << " at " << node.getLine();
			return -1;
		}

		blueprints[blueprint->getId()] = blueprint;
		FO_READ_ERROR
		return -1;
		FO_READ_END
	}

	return 0;
}

void _MObjectBP::shutdown()
{
	for (BlueprintMap::iterator i = blueprints.begin(), e = blueprints.end();
	        i != e; ++i)
		delete i->second;
}

ObjectBP* _MObjectBP::lookup(const std::string& id)
{
	BlueprintMap::iterator iter = blueprints.find(id);
	if (iter == blueprints.end())
		return NULL;
	else
		return iter->second;
}
