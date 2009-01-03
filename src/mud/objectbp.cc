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
#include "mud/shadow-object.h"

_MObjectBP MObjectBP;

ObjectBP::ObjectBP()
{
	weight = 0;
	cost = 0;
}

bool
ObjectBP::set_name(const std::string& s_name)
{
	bool ret = name.set_name(s_name);
	return ret;
}

EntityName
ObjectBP::get_name() const
{
	return name;
}

bool
ObjectBP::has_tag(TagID tag) const
{
	return tags.find(tag) != tags.end();
}

int
ObjectBP::add_tag(TagID tag)
{
	tags.insert(tag);
	return 0;
}

int
ObjectBP::remove_tag(TagID tag)
{
	// find
	TagList::iterator ti = std::find(tags.begin(), tags.end(), tag);
	if (ti == tags.end())
		return 1;

	// remove
	tags.erase(ti);

	return 0;
}

void
ObjectBP::save(File::Writer& writer)
{
	if (!id.empty())
		writer.attr("blueprint", "id", id);

	writer.attr("blueprint", "name", name.get_name());
	writer.attr("blueprint", "desc", desc);

	for (std::vector<std::string>::const_iterator i = keywords.begin(); i != keywords.end(); ++i)
		writer.attr("blueprint", "keyword", *i);

	writer.attr("blueprint", "equip", equip.get_name());

	writer.attr("blueprint", "cost", cost);
	writer.attr("blueprint", "weight", weight);

	writer.attr("blueprint", "hidden", flags.check(ObjectFlag::HIDDEN));
	writer.attr("blueprint", "gettable", flags.check(ObjectFlag::GET));
	writer.attr("blueprint", "touchable", flags.check(ObjectFlag::TOUCH));
	writer.attr("blueprint", "dropable", flags.check(ObjectFlag::DROP));
	writer.attr("blueprint", "trashable", flags.check(ObjectFlag::TRASH));
	writer.attr("blueprint", "rotting", flags.check(ObjectFlag::ROT));

	if (locations.check(ObjectLocation::IN))
		writer.attr("blueprint", "container", "in");
	if (locations.check(ObjectLocation::ON))
		writer.attr("blueprint", "container", "on");

	for (TagList::iterator i = tags.begin(); i != tags.end(); ++i)
		writer.attr("blueprint", "tag", i->name());

	// script hook
	Hooks::save_object_blueprint(this, writer);
}

int
ObjectBP::load(File::Reader& reader)
{
	FO_READ_BEGIN
	FO_ATTR("blueprint", "id")
	id = node.get_string();
	FO_ATTR("blueprint", "name")
	set_name(node.get_string());
	FO_ATTR("blueprint", "keyword")
	keywords.push_back(node.get_string());
	FO_ATTR("blueprint", "desc")
	set_desc(node.get_string());
	FO_ATTR("blueprint", "weight")
	set_weight(node.get_int());
	FO_ATTR("blueprint", "cost")
	set_cost(node.get_int());
	FO_ATTR("blueprint", "equip")
	set_equip(EquipSlot::lookup(node.get_string()));
	FO_ATTR("blueprint", "gettable")
	set_flag(ObjectFlag::GET, node.get_bool());
	FO_ATTR("blueprint", "touchable")
	set_flag(ObjectFlag::TOUCH, node.get_bool());
	FO_ATTR("blueprint", "hidden")
	set_flag(ObjectFlag::HIDDEN, node.get_bool());
	FO_ATTR("blueprint", "dropable")
	set_flag(ObjectFlag::DROP, node.get_bool());
	FO_ATTR("blueprint", "trashable")
	set_flag(ObjectFlag::TRASH, node.get_bool());
	FO_ATTR("blueprint", "rotting")
	set_flag(ObjectFlag::ROT, node.get_bool());
	FO_ATTR("blueprint", "container")
	if (node.get_string() == "on") {
		locations.set_on(ObjectLocation::ON);
	} else if (node.get_string() == "in") {
		locations.set_on(ObjectLocation::IN);
	} else
		Log::Warning << "Unknown container type '" << node.get_string() << "' at " << reader.get_filename() << ':' << node.get_line();
	FO_ATTR("blueprint", "tag")
	tags.insert(TagID::create(node.get_string()));
	FO_READ_ERROR
	return -1;
	FO_READ_END

	return 0;
}

int
_MObjectBP::initialize()
{
	// requirements
	if (require(MEvent) != 0)
		return 1;

	std::vector<std::string> files = File::dirlist(MSettings.get_blueprint_path());
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
			Log::Warning << "Failed to load blueprint in " << reader.get_filename() << " at " << node.get_line();
			return -1;
		}

		if (blueprint->get_id().empty()) {
			Log::Warning << "Blueprint has no ID in " << reader.get_filename() << " at " << node.get_line();
			return -1;
		}

		blueprints[blueprint->get_id()] = blueprint;
		FO_READ_ERROR
		return -1;
		FO_READ_END
	}

	return 0;
}

void
_MObjectBP::shutdown()
{
	for (BlueprintMap::iterator i = blueprints.begin(), e = blueprints.end();
	        i != e; ++i)
		delete i->second;
}

ObjectBP*
_MObjectBP::lookup(const std::string& id)
{
	BlueprintMap::iterator iter = blueprints.find(id);
	if (iter == blueprints.end())
		return NULL;
	else
		return iter->second;
}
