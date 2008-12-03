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
#include "mud/entity.h"
#include "mud/object.h"
#include "mud/creature.h"
#include "mud/room.h"
#include "mud/server.h"
#include "mud/body.h"
#include "mud/player.h"
#include "mud/settings.h"
#include "mud/hooks.h"
#include "mud/shadow-object.h"
#include "mud/unique-object.h"
#include "mud/efactory.h"

std::string ObjectLocation::names[] = {
	S("none"),
	S("in"),
	S("on"),
	S("MAX")
};

// ----- ObjectBP -----

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
		writer.attr(S("blueprint"), S("id"), id);

	writer.attr(S("blueprint"), S("name"), name.get_name());
	writer.attr(S("blueprint"), S("desc"), desc);

	for (StringList::const_iterator i = keywords.begin(); i != keywords.end(); ++i)
		writer.attr(S("blueprint"), S("keyword"), *i);

	writer.attr(S("blueprint"), S("equip"), equip.get_name());

	writer.attr(S("blueprint"), S("cost"), cost);
	writer.attr(S("blueprint"), S("weight"), weight);

	writer.attr(S("blueprint"), S("hidden"), flags.check(ObjectFlag::HIDDEN));
	writer.attr(S("blueprint"), S("gettable"), flags.check(ObjectFlag::GET));
	writer.attr(S("blueprint"), S("touchable"), flags.check(ObjectFlag::TOUCH));
	writer.attr(S("blueprint"), S("dropable"), flags.check(ObjectFlag::DROP));
	writer.attr(S("blueprint"), S("trashable"), flags.check(ObjectFlag::TRASH));
	writer.attr(S("blueprint"), S("rotting"), flags.check(ObjectFlag::ROT));

	if (locations.check(ObjectLocation::IN))
		writer.attr(S("blueprint"), S("container"), S("in"));
	if (locations.check(ObjectLocation::ON))
		writer.attr(S("blueprint"), S("container"), S("on"));

	for (TagList::iterator i = tags.begin(); i != tags.end(); ++i)
		writer.attr(S("blueprint"), S("tag"), i->name());

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
			if (node.get_string() == S("on")) {
				locations.set_on(ObjectLocation::ON);
			} else if (node.get_string() == S("in")) {
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

// ----- Object -----

Object::Object()
{
	owner = NULL;
	calc_weight = 0;
	trash_timer = 0;
}

ShadowObject::ShadowObject() : Object(), blueprint(0) { }

ShadowObject::ShadowObject (ObjectBP* s_blueprint) : Object(), blueprint(0)
{
	set_blueprint(s_blueprint);
}

UniqueObject::UniqueObject() : Object() { }

Object::~Object() { }
ShadowObject::~ShadowObject() { }
UniqueObject::~UniqueObject() { }

void
Object::save_data(File::Writer& writer)
{
	// parent location
	if (container == ObjectLocation::IN)
		writer.attr(S("object"), S("location"), S("in"));
	if (container == ObjectLocation::ON)
		writer.attr(S("object"), S("location"), S("on"));

	// save children objects
	for (EList<Object>::const_iterator e = children.begin(); e != children.end(); ++e) {
		(*e)->save(writer, S("object"), S("child"));
	}

	// parent data
	Entity::save_data(writer);
}

void
UniqueObject::save_data(File::Writer& writer)
{
	writer.attr(S("object"), S("name"), name.get_name());
	writer.attr(S("object"), S("desc"), desc);

	for (StringList::const_iterator i = keywords.begin(); i != keywords.end(); ++i)
		writer.attr(S("object"), S("keyword"), *i);

	writer.attr(S("object"), S("equip"), equip.get_name());

	writer.attr(S("object"), S("cost"), cost);
	writer.attr(S("object"), S("weight"), weight);

	writer.attr(S("object"), S("hidden"), flags.check(ObjectFlag::HIDDEN));
	writer.attr(S("object"), S("gettable"), flags.check(ObjectFlag::GET));
	writer.attr(S("object"), S("touchable"), flags.check(ObjectFlag::TOUCH));
	writer.attr(S("object"), S("dropable"), flags.check(ObjectFlag::DROP));
	writer.attr(S("object"), S("trashable"), flags.check(ObjectFlag::TRASH));
	writer.attr(S("object"), S("rotting"), flags.check(ObjectFlag::ROT));

	if (locations.check(ObjectLocation::IN))
		writer.attr(S("object"), S("container"), S("in"));
	if (locations.check(ObjectLocation::ON))
		writer.attr(S("object"), S("container"), S("on"));

	for (TagList::iterator i = tags.begin(); i != tags.end(); ++i)
		writer.attr(S("object"), S("tag"), i->name());

	// parent data
	Object::save_data(writer);
}

void
ShadowObject::save_data(File::Writer& writer)
{
	// save blueprint
	if (get_blueprint())
		writer.attr(S("object"), S("blueprint"), get_blueprint()->get_id());

	// save name, if set
	if (!name.empty())
		writer.attr(S("object"), S("name"), name.get_name());

	// parent data
	Object::save_data(writer);
}

void
Object::save_hook(File::Writer& writer)
{
	Entity::save_hook(writer);
	Hooks::save_object(this, writer);
}

int
Object::load_finish()
{
	recalc_weight();
	return 0;
}

int
ShadowObject::load_finish()
{
	if (blueprint == NULL) {
		Log::Error << "object has no blueprint";
		return -1;
	}

	return Object::load_finish();
}

int
Object::load_node(File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_ATTR("object", "location")
			if (node.get_string() == S("in"))
				container = ObjectLocation::IN;
			else if (node.get_string() == S("on"))
				container = ObjectLocation::ON;
			else
				throw File::Error(S("Object has invalid container attribute"));
		FO_ENTITY("object", "child")
			if (OBJECT(entity) == NULL) throw File::Error(S("Object child is not an Object"));
			OBJECT(entity)->set_owner (this);
			children.add (OBJECT(entity));
		FO_PARENT(Entity)
	FO_NODE_END
}

int
ShadowObject::load_node(File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_ATTR("object", "blueprint")
			// sets a real blueprint
			ObjectBP* blueprint = NULL;
			if ((blueprint = MObjectBP.lookup(node.get_string())) == NULL)
				Log::Error << "Could not find object blueprint '" << node.get_string() << "'";
			else
				set_blueprint(blueprint);
		FO_ATTR("object", "name")
			name.set_name(node.get_string());
		FO_PARENT(Entity)
	FO_NODE_END
}

int
UniqueObject::load_node(File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_ATTR("object", "name")
			set_name(node.get_string());
		FO_ATTR("object", "keyword")
			keywords.push_back(node.get_string());
		FO_ATTR("object", "desc")
			set_desc(node.get_string());
		FO_ATTR("object", "weight")
			set_weight(node.get_int());
		FO_ATTR("object", "cost")
			set_cost(node.get_int());
		FO_ATTR("object", "equip")
			set_equip(EquipSlot::lookup(node.get_string()));
		FO_ATTR("object", "gettable")
			set_flag(ObjectFlag::GET, node.get_bool());
		FO_ATTR("object", "touchable")
			set_flag(ObjectFlag::TOUCH, node.get_bool());
		FO_ATTR("object", "hidden")
			set_flag(ObjectFlag::HIDDEN, node.get_bool());
		FO_ATTR("object", "dropable")
			set_flag(ObjectFlag::DROP, node.get_bool());
		FO_ATTR("object", "trashable")
			set_flag(ObjectFlag::TRASH, node.get_bool());
		FO_ATTR("object", "rotting")
			set_flag(ObjectFlag::ROT, node.get_bool());
		FO_ATTR("object", "container")
			if (node.get_string() == S("on")) {
				locations.set_on(ObjectLocation::ON);
			} else if (node.get_string() == S("in")) {
				locations.set_on(ObjectLocation::IN);
			} else
				Log::Warning << "Unknown container type '" << node.get_string() << "' at " << reader.get_filename() << ':' << node.get_line();
		FO_PARENT(Object)
	FO_NODE_END
}

void
Object::set_owner(Entity* s_owner)
{
	// type check
	assert(OBJECT(s_owner) || ROOM(s_owner) || CHARACTER(s_owner));

	// set owner
	Entity::set_owner(s_owner);
	owner = s_owner;
}

void
Object::owner_release(Entity* child)
{
	// we only own objects
	Object* obj = OBJECT(child);
	assert(obj != NULL);

	// find it
	EList<Object>::iterator e = std::find(children.begin(), children.end(), obj);
	if (e != children.end()) {
		obj->container = ObjectLocation::NONE;
		children.erase(e);
		return;
	}
}

void
Object::heartbeat()
{
	// see if we can trash the object
	if (is_trashable()) {
		// must be laying in a room
		Room* room = ROOM(get_owner());
		if (room != NULL) {
			// rotting?
			if (is_rotting() && trash_timer >= OBJECT_ROT_TICKS) {
				// destroy it
				*room << StreamName(this, INDEFINITE, true) << " rots away.\n";
				destroy();

			// not rotting - normal trash
			} else if (trash_timer >= OBJECT_TRASH_TICKS) {
				// room must not have any players in it
				if (room->count_players() == 0) {
					// destroy it
					destroy();
				}
			} else {
				++trash_timer;
			}
		}
	}

	// call update hook
	Hooks::object_heartbeat(this);
}

void
Object::activate()
{
	Entity::activate();

	for (EList<Object>::iterator e = children.begin(); e != children.end(); ++e)
		(*e)->activate();
}

void
Object::deactivate()
{
	for (EList<Object>::iterator e = children.begin(); e != children.end(); ++e)
		(*e)->deactivate();

	Entity::deactivate();
}

bool
Object::add_object(Object *object, ObjectLocation container)
{
	assert (object != NULL);

	// has contianer?
	if (!has_location(container))
		return false;

	// release and add
	object->set_owner(this);
	object->container = container;
	children.add(object);

	// recalc our weight, and parent's weight
	recalc_weight();
	if (OBJECT(owner))
		((Object*)owner)->recalc_weight();

	// ok add
	return true;
}

void
Object::show_contents(Player *player, ObjectLocation container) const
{
	*player << "You see ";
	
	Object* last = NULL;
	int displayed = 0;

	// show objects
	for (EList<Object>::const_iterator i = children.begin(); i != children.end(); ++i) {
		// not right container?
		if ((*i)->container != container)
			continue;
		// had a previous item?
		if (last != NULL) {
			// first item?
			if (displayed)
				*player << ", ";
			*player << StreamName(last, INDEFINITE, false);
			++displayed;
		}
		last = *i;
	}
	// one more item?
	if (last != NULL) {
		if (displayed > 1)
			*player << ", and ";
		else if (displayed == 1)
			*player << " and ";
		*player << StreamName(last, INDEFINITE, false);
		++displayed;
	}

	// no items?
	if (!displayed)
		*player << "nothing";

	// finish up
	std::string tname = S("somewhere on");
	if (container == ObjectLocation::ON)
		tname = S("on");
	else if (container == ObjectLocation::IN)
		tname = S("in");
	*player << " " << tname << " " << StreamName(*this, DEFINITE, false) << ".\n";
}

Object *
Object::find_object(const std::string& name, uint index, ObjectLocation container, uint *matches) const
{
	assert (index != 0);

	// clear matches
	if (matches)
		*matches = 0;
	
	for (EList<Object>::const_iterator i = children.begin(); i != children.end(); ++i) {
		// right container container
		if ((*i)->container == container) {
			// check name
			if ((*i)->name_match (name)) {
				if (matches)
					++ *matches;
				if ((-- index) == 0)
					return OBJECT((*i));
			}
		}
	}

	// not found
	return NULL;
}

// recalc weight of object
void
Object::recalc_weight()
{
	calc_weight = 0;

	// add up weight of objects
	for (EList<Object>::const_iterator i = children.begin(); i != children.end(); ++i)
		calc_weight += (*i)->get_weight();
}

// find parent room
Room*
Object::get_room() const
{
	Entity* owner = get_owner();
	while (owner != NULL && !ROOM(owner))
		owner = owner->get_owner();
	return ROOM(owner);
}

// find parent owner
Creature* 
Object::get_holder() const
{
	Entity* owner = get_owner();
	while (owner != NULL && OBJECT(owner))
		owner = owner->get_owner();
	return CHARACTER(owner);
}

bool
ShadowObject::set_name(const std::string& s_name)
{
	bool ret = name.set_name(s_name);
	return ret;
}

bool
UniqueObject::set_name(const std::string& s_name)
{
	bool ret = name.set_name(s_name);
	return ret;
}

// get object name information
EntityName
ShadowObject::get_name() const
{
	assert(blueprint != NULL);
	if (name.empty())
		return blueprint->get_name();
	else
		return name;
}

// get object description
std::string
ShadowObject::get_desc() const
{
	assert(blueprint != NULL);
	return blueprint->get_desc();
}

// get object properties
uint
ShadowObject::get_cost() const
{
	assert(blueprint != NULL);
	return blueprint->get_cost();
}
uint
ShadowObject::get_real_weight() const
{
	assert(blueprint != NULL);
	return blueprint->get_weight();
}
EquipSlot
ShadowObject::get_equip() const
{
	assert(blueprint != NULL);
	return blueprint->get_equip();
}

// get flags
bool
ShadowObject::is_hidden() const
{
	assert(blueprint != NULL);
	return blueprint->get_flag(ObjectFlag::HIDDEN);
}
bool
ShadowObject::is_trashable() const
{
	assert(blueprint != NULL);
	return blueprint->get_flag(ObjectFlag::TRASH);
}
bool
ShadowObject::is_gettable() const
{
	assert(blueprint != NULL);
	return blueprint->get_flag(ObjectFlag::GET);
}
bool
ShadowObject::is_dropable() const
{
	assert(blueprint != NULL);
	return blueprint->get_flag(ObjectFlag::DROP);
}
bool
ShadowObject::is_touchable() const
{
	assert(blueprint != NULL);
	return blueprint->get_flag(ObjectFlag::TOUCH);
}
bool
ShadowObject::is_rotting() const
{
	assert(blueprint != NULL);
	return blueprint->get_flag(ObjectFlag::ROT);
}

// get parsable member values
int
Object::macro_property(const StreamControl& stream, const std::string& comm, const MacroList& argv) const
{
	// COST
	if (str_eq(comm, "cost")) {
		stream << get_cost();
		return 0;
	}
	// WEIGHT
	if (str_eq(comm, "weight")) {
		stream << get_weight();
		return 0;
	}
	// try the entity
	return Entity::macro_property(stream, comm, argv);
}

// event handling
void
Object::handle_event(const Event& event)
{
	Entity::handle_event(event);
}

void
Object::broadcast_event(const Event& event)
{
	for (EList<Object>::const_iterator i = children.begin(); i != children.end(); ++i)
		MEvent.resend(event, *i);
}

void
ShadowObject::set_blueprint(ObjectBP* s_blueprint)
{
	blueprint = s_blueprint;
}

// load object from a blueprint
Object*
ShadowObject::load_blueprint(const std::string& name)
{
	ObjectBP* blueprint = MObjectBP.lookup(name);
	if (!blueprint)
		return NULL;
	
	return new ShadowObject(blueprint);
}

bool
ShadowObject::is_blueprint(const std::string& name) const
{
	ObjectBP* blueprint = get_blueprint();

	if (blueprint != NULL)
		return str_eq(blueprint->get_id(), name);

	return false;
}

bool
ShadowObject::name_match(const std::string& match) const
{
	if (get_name().matches(match))
		return true;

	// blueprint keywords
	ObjectBP* blueprint = get_blueprint();
	if (blueprint != NULL) {
		for (StringList::const_iterator i = blueprint->get_keywords().begin(); i != blueprint->get_keywords().end(); ++i)
			if (phrase_match (*i, match))
				return true;
	}

	// no match
	return false;
}

// Object Blueprint Manager

_MObjectBP MObjectBP;

int
_MObjectBP::initialize()
{
	// requirements
	if (require(MEvent) != 0)
		return 1;

	StringList files = File::dirlist(MSettings.get_blueprint_path());
	File::filter(files, "*.objs");
	for (StringList::iterator i = files.begin(); i != files.end(); ++i) {
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

BEGIN_EFACTORY(SObject)
	return new ShadowObject();
END_EFACTORY

BEGIN_EFACTORY(UObject)
	return new UniqueObject();
END_EFACTORY
