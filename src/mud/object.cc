/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "mud/entity.h"
#include "mud/object.h"
#include "common/string.h"
#include "common/error.h"
#include "mud/creature.h"
#include "mud/room.h"
#include "mud/server.h"
#include "mud/body.h"
#include "mud/player.h"
#include "common/streams.h"
#include "mud/settings.h"
#include "mud/hooks.h"
#include "common/manifest.h"

String ObjectLocation::names[] = {
	S("none"),
	S("in"),
	S("on"),
	S("MAX")
};

// ----- ObjectBP -----

SCRIPT_TYPE(ObjectBP);
ObjectBP::ObjectBP () : Scriptix::Native(AweMUD_ObjectBPType), parent(NULL)
{
	weight = 0;
	cost = 0;
}

bool
ObjectBP::set_name (String s_name)
{
	bool ret = name.set_name(s_name);
	value_set &= ObjectBPSet::NAME;
	return ret;
}

EntityName
ObjectBP::get_name () const
{
	return name;
}

void
ObjectBP::reset_name ()
{
	// clear
	name.set_name(S("an object"));
	value_set.set_off(ObjectBPSet::NAME);

	// get parent value
	const ObjectBP* data = get_parent();
	if (data != NULL)
		name = data->get_name();
}

void
ObjectBP::reset_desc ()
{
	// clear
	desc = S("object");
	value_set.set_off(ObjectBPSet::DESC);

	// get parent value
	const ObjectBP* data = get_parent();
	if (data != NULL)
		desc = data->get_desc();
}

void
ObjectBP::reset_weight ()
{
	// clear
	weight = 0;
	value_set.set_off(ObjectBPSet::WEIGHT);

	// get parent value
	const ObjectBP* data = get_parent();
	if (data != NULL)
		weight = data->get_weight();
}

void
ObjectBP::reset_cost ()
{
	// clear
	cost = 0;
	value_set.set_off(ObjectBPSet::COST);

	// get parent value
	const ObjectBP* data = get_parent();
	if (data != NULL)
		cost = data->get_cost();
}

void
ObjectBP::reset_equip ()
{
	// clear
	equip = 0;
	value_set.set_off(ObjectBPSet::EQUIP);

	// get parent value
	const ObjectBP* data = get_parent();
	if (data != NULL)
		equip = data->get_equip();
}

void
ObjectBP::reset_flag (ObjectFlag flag)
{
	flags_set.set_off(flag);

	// get parent value
	const ObjectBP* data = get_parent();
	if (data != NULL)
		flags.set(flag, data->get_flag(flag));
}

void
ObjectBP::refresh ()
{
	if (!value_set.check(ObjectBPSet::NAME))
		reset_name();
	if (!value_set.check(ObjectBPSet::DESC))
		reset_desc();
	if (!value_set.check(ObjectBPSet::WEIGHT))
		reset_weight();
	if (!value_set.check(ObjectBPSet::COST))
		reset_cost();
	if (!value_set.check(ObjectBPSet::EQUIP))
		reset_equip();
	if (!flags_set.check(ObjectFlag::HIDDEN))
		reset_flag(ObjectFlag::HIDDEN);
	if (!flags_set.check(ObjectFlag::GET))
		reset_flag(ObjectFlag::GET);
	if (!flags_set.check(ObjectFlag::TOUCH))
		reset_flag(ObjectFlag::TOUCH);
	if (!flags_set.check(ObjectFlag::DROP))
		reset_flag(ObjectFlag::DROP);
	if (!flags_set.check(ObjectFlag::TRASH))
		reset_flag(ObjectFlag::TRASH);
	if (!flags_set.check(ObjectFlag::ROT))
		reset_flag(ObjectFlag::ROT);
}

void
ObjectBP::save (File::Writer& writer)
{
	if (id)
		writer.attr(S("blueprint"), S("id"), id);

	if (value_set.check(ObjectBPSet::NAME))
		writer.attr(S("blueprint"), S("name"), name.get_name());

	if (value_set.check(ObjectBPSet::DESC))
		writer.attr(S("blueprint"), S("desc"), desc);

	for (StringList::const_iterator i = keywords.begin(); i != keywords.end(); ++i)
		writer.attr(S("blueprint"), S("keyword"), *i);

	if (value_set.check(ObjectBPSet::EQUIP))
		writer.attr(S("blueprint"), S("equip"), equip.get_name());

	if (value_set.check(ObjectBPSet::COST))
		writer.attr(S("blueprint"), S("cost"), cost);
	if (value_set.check(ObjectBPSet::WEIGHT))
		writer.attr(S("blueprint"), S("weight"), weight);

	if (flags_set.check(ObjectFlag::HIDDEN))
		writer.attr(S("blueprint"), S("hidden"), flags.check(ObjectFlag::HIDDEN));
	if (flags_set.check(ObjectFlag::GET))
		writer.attr(S("blueprint"), S("gettable"), flags.check(ObjectFlag::GET));
	if (flags_set.check(ObjectFlag::TOUCH))
		writer.attr(S("blueprint"), S("touchable"), flags.check(ObjectFlag::TOUCH));
	if (flags_set.check(ObjectFlag::DROP))
		writer.attr(S("blueprint"), S("dropable"), flags.check(ObjectFlag::DROP));
	if (flags_set.check(ObjectFlag::TRASH))
		writer.attr(S("blueprint"), S("trashable"), flags.check(ObjectFlag::TRASH));
	if (flags_set.check(ObjectFlag::ROT))
		writer.attr(S("blueprint"), S("rotting"), flags.check(ObjectFlag::ROT));

	if (parent)
		writer.attr(S("blueprint"), S("parent"), parent->get_id());

	if (locations_set.check(ObjectLocation::IN))
		writer.attr(S("blueprint"), S("container"), S("in"));
	if (locations_set.check(ObjectLocation::ON))
		writer.attr(S("blueprint"), S("container"), S("on"));

	// script hook
	ScriptRestrictedWriter* swriter = new ScriptRestrictedWriter(&writer);
	Hooks::save_object_blueprint(this, swriter);
	swriter->release();
	swriter = NULL;
}

int
ObjectBP::load (File::Reader& reader)
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
				locations_set.set_on(ObjectLocation::ON);
			} else if (node.get_string() == S("in")) {
				locations.set_on(ObjectLocation::IN);
				locations_set.set_on(ObjectLocation::IN);
			}
			else
				Log::Warning << "Unknown container type '" << node.get_string() << "' at " << reader.get_filename() << ':' << node.get_line();
		FO_ATTR("blueprint", "parent")
			ObjectBP* blueprint = ObjectBPManager.lookup(node.get_string());
			if (blueprint)
				set_parent(blueprint);
			else
				Log::Warning << "Undefined parent object blueprint '" << node.get_string() << "' at " << reader.get_filename() << ':' << node.get_line();
		FO_WILD("user")
			if (node.get_value_type() == File::Value::TYPE_INT)
				set_property(node.get_name(), node.get_int());
			else if (node.get_value_type() == File::Value::TYPE_STRING)
				set_property(node.get_name(), node.get_string());
			else if (node.get_value_type() == File::Value::TYPE_BOOL)
				set_property(node.get_name(), node.get_bool());
			else {
				Log::Error << "Invalid data type for script attribute at " << reader.get_filename() << ':' << node.get_line();
				return -1;
			}
	FO_READ_ERROR
		return -1;
	FO_READ_END

	return 0;
}

void
ObjectBP::set_parent (ObjectBP* blueprint)
{
	parent = blueprint;
	refresh();
}

Scriptix::Value
ObjectBP::get_undefined_property (Scriptix::Atom id) const
{
	if (parent == NULL)
		return Scriptix::Nil;
	return parent->get_property(id);
}

// ----- Object -----

SCRIPT_TYPE(Object);
Object::Object () : Entity (AweMUD_ObjectType)
{
	blueprint = NULL;
	owner = NULL;
	calc_weight = 0;
	trash_timer = 0;
}

Object::Object (ObjectBP* s_blueprint) : Entity(AweMUD_ObjectType)
{
	blueprint = NULL;
	owner = NULL;
	calc_weight = 0;
	set_blueprint(s_blueprint);
	trash_timer = 0;
}

Object::~Object ()
{
}

void
Object::save (File::Writer& writer)
{
	// save blueprint
	if (get_blueprint()) {
		// real blueprint
		if (get_blueprint()->get_id()) {
			writer.attr(S("object"), S("blueprint"), get_blueprint()->get_id());
		// anonymous blueprint
		} else {
			writer.begin(S("blueprint"));
			get_blueprint()->save(writer);
			writer.end();
		}
	}

	// save name, if set
	if (!name.empty())
		writer.attr(S("object"), S("name"), name.get_name());

	// parent data
	Entity::save(writer);

	if (container == ObjectLocation::IN)
		writer.attr(S("object"), S("container"), S("in"));
	if (container == ObjectLocation::ON)
		writer.attr(S("object"), S("container"), S("on"));

	for (EList<Object>::const_iterator e = children.begin (); e != children.end(); ++e) {
		writer.begin(S("object"));
		(*e)->save (writer);
		writer.end();
	}
}

void
Object::save_hook (ScriptRestrictedWriter* writer)
{
	Entity::save_hook(writer);
	Hooks::save_object(this, writer);
}

int
Object::load_finish ()
{
	recalc_weight();
	
	if (blueprint == NULL) {
		Log::Error << "object has no blueprint";
		return -1;
	}

	return 0;
}

int
Object::load_node(File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_OBJECT("blueprint")
			// creates a new anonymous blueprint
			ObjectBP* blueprint = new ObjectBP();
			if (blueprint->load(reader))
				throw File::Error(S("Failed to load anonymous blueprint"));
			set_blueprint(blueprint);
		FO_ATTR("object", "blueprint")
			// sets a real blueprint
			ObjectBP* blueprint = NULL;
			if ((blueprint = ObjectBPManager.lookup(node.get_string())) == NULL)
				Log::Error << "Could not find object blueprint '" << node.get_string() << "'";
			else
				set_blueprint(blueprint);
		FO_ATTR("object", "name")
			name.set_name(node.get_string());
		FO_ATTR("object", "container")
			if (node.get_string() == S("in"))
				container = ObjectLocation::IN;
			else if (node.get_string() == S("on"))
				container = ObjectLocation::ON;
			else
				throw File::Error(S("Object has invalid container attribute"));
		FO_OBJECT("object")
			Object* obj = new Object ();
			if (obj->load (reader))
				throw File::Error(S("Failed to load object"));
			if (!has_location(obj->container))
				throw File::Error(S("child object unallowed container"));
			obj->set_owner (this);
			children.add (obj);
		FO_PARENT(Entity)
	FO_NODE_END
}

void
Object::set_owner (Entity* s_owner)
{
	// type check
	assert(OBJECT(s_owner) || ROOM(s_owner) || CHARACTER(s_owner));

	// set owner
	Entity::set_owner(s_owner);
	owner = s_owner;
}

void
Object::owner_release (Entity* child)
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
Object::activate ()
{
	Entity::activate();

	for (EList<Object>::iterator e = children.begin (); e != children.end(); ++e)
		(*e)->activate();
}

void
Object::deactivate ()
{
	for (EList<Object>::iterator e = children.begin (); e != children.end(); ++e)
		(*e)->deactivate();

	Entity::deactivate();
}

bool
Object::add_object (Object *object, ObjectLocation container)
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
Object::show_contents (Player *player, ObjectLocation container) const
{
	*player << "You see ";
	
	Object* last = NULL;
	int displayed = 0;

	// show objects
	for (EList<Object>::const_iterator i = children.begin (); i != children.end(); ++i) {
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
	String tname = S("somewhere on");
	if (container == ObjectLocation::ON)
		tname = S("on");
	else if (container == ObjectLocation::IN)
		tname = S("in");
	*player << " " << tname << " " << StreamName(*this, DEFINITE, false) << ".\n";
}

Object *
Object::find_object (String name, uint index, ObjectLocation container, uint *matches) const
{
	assert (index != 0);

	// clear matches
	if (matches)
		*matches = 0;
	
	for (EList<Object>::const_iterator i = children.begin (); i != children.end(); ++i) {
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
Object::recalc_weight ()
{
	calc_weight = 0;

	// add up weight of objects
	for (EList<Object>::const_iterator i = children.begin(); i != children.end(); ++i)
		calc_weight += (*i)->get_weight();
}

// find parent room
Room*
Object::get_room () const
{
	Entity* owner = get_owner();
	while (owner != NULL && !ROOM(owner))
		owner = owner->get_owner();
	return ROOM(owner);
}

// find parent owner
Creature* 
Object::get_holder () const
{
	Entity* owner = get_owner();
	while (owner != NULL && !CHARACTER(owner))
		owner = owner->get_owner();
	return CHARACTER(owner);
}

bool
Object::set_name (String s_name)
{
	bool ret = name.set_name(s_name);
	return ret;
}

// get object name information
EntityName
Object::get_name () const
{
	assert(blueprint != NULL);
	if (name.empty())
		return blueprint->get_name();
	else
		return name;
}

// get object description
String
Object::get_desc () const
{
	assert(blueprint != NULL);
	return blueprint->get_desc();
}

// get object properties
uint
Object::get_cost () const
{
	assert(blueprint != NULL);
	return blueprint->get_cost();
}
uint
Object::get_real_weight () const
{
	assert(blueprint != NULL);
	return blueprint->get_weight();
}
EquipSlot
Object::get_equip () const
{
	assert(blueprint != NULL);
	return blueprint->get_equip();
}

// get flags
bool
Object::is_hidden () const
{
	assert(blueprint != NULL);
	return blueprint->get_flag(ObjectFlag::HIDDEN);
}
bool
Object::is_trashable () const
{
	assert(blueprint != NULL);
	return blueprint->get_flag(ObjectFlag::TRASH);
}
bool
Object::is_gettable () const
{
	assert(blueprint != NULL);
	return blueprint->get_flag(ObjectFlag::GET);
}
bool
Object::is_dropable () const
{
	assert(blueprint != NULL);
	return blueprint->get_flag(ObjectFlag::DROP);
}
bool
Object::is_touchable () const
{
	assert(blueprint != NULL);
	return blueprint->get_flag(ObjectFlag::TOUCH);
}
bool
Object::is_rotting () const
{
	assert(blueprint != NULL);
	return blueprint->get_flag(ObjectFlag::ROT);
}

// get parsable member values
int
Object::parse_property (const StreamControl& stream, String comm, const ParseList& argv) const
{
	// COST
	if (!strcmp(comm, "cost")) {
		stream << get_cost();
		return 0;
	}
	// WEIGHT
	if (!strcmp(comm, "weight")) {
		stream << get_weight();
		return 0;
	}
	// try the entity
	return Entity::parse_property(stream, comm, argv);
}

void
Object::set_blueprint (ObjectBP* s_blueprint)
{
	blueprint = s_blueprint;
}

// load object from a blueprint
Object*
Object::load_blueprint (String name)
{
	ObjectBP* blueprint = ObjectBPManager.lookup(name);
	if (!blueprint)
		return NULL;
	
	return new Object(blueprint);
}

bool
Object::is_blueprint (String name) const
{
	ObjectBP* blueprint = get_blueprint();

	while (blueprint != NULL) {
		if (str_eq(blueprint->get_id(), name))
			return true;

		blueprint = blueprint->get_parent();
	}

	return false;
}

bool
Object::name_match (String match) const
{
	if (get_name().matches(match))
		return true;

	// blueprint keywords
	ObjectBP* blueprint = get_blueprint();
	while (blueprint != NULL) {
		for (StringList::const_iterator i = blueprint->get_keywords().begin(); i != blueprint->get_keywords().end(); i ++)
			if (phrase_match (*i, match))
				return true;

		blueprint = blueprint->get_parent();
	}

	// no match
	return false;
}

Scriptix::Value
Object::get_undefined_property (Scriptix::Atom id) const
{
	const ObjectBP* data = get_blueprint();
	if (data == NULL)
		return Scriptix::Nil;
	return data->get_property(id);
}

// Object Blueprint Manager

SObjectBPManager ObjectBPManager;

int
SObjectBPManager::initialize ()
{
	// requirements
	if (require(ScriptBindings) != 0)
		return 1;
	if (require(EventManager) != 0)
		return 1;


	ManifestFile man(SettingsManager.get_blueprint_path(), S(".objs"));
	StringList files = man.get_files();;
	for (StringList::iterator i = files.begin(); i != files.end(); ++i) {
		if (has_suffix(*i, S(".objs"))) {
			// load from file
			File::Reader reader;
			if (reader.open(*i))
				return -1;
			FO_READ_BEGIN
				FO_OBJECT("object_blueprint")
					ObjectBP* blueprint = new ObjectBP();
					if (blueprint->load(reader)) {
						Log::Warning << "Failed to load blueprint in " << reader.get_filename() << " at " << node.get_line();
						return -1;
					}

					if (!blueprint->get_id()) {
						Log::Warning << "Blueprint has no ID in " << reader.get_filename() << " at " << node.get_line();
						return -1;
					}

					blueprints[blueprint->get_id()] = blueprint;
			FO_READ_ERROR
				return -1;
			FO_READ_END
		}
	}

	return 0;
}

void
SObjectBPManager::shutdown ()
{
}

ObjectBP*
SObjectBPManager::lookup (String id)
{
	BlueprintMap::iterator iter = blueprints.find(id);
	if (iter == blueprints.end())
		return NULL;
	else
		return iter->second;
}
