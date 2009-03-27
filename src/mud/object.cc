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
#include "mud/object.h"
#include "mud/creature.h"
#include "mud/room.h"
#include "mud/server.h"
#include "mud/player.h"
#include "mud/settings.h"
#include "mud/hooks.h"
#include "mud/efactory.h"

std::string ObjectLocation::names[] = {
	"none",
	"in",
	"on",
	"MAX"
};

Object::Object() : owner(0), blueprint(0), calc_weight(0), trash_timer(0)
{
	blueprint = new ObjectBP();
}

Object::Object(ObjectBP* s_blueprint) : owner(0), blueprint(s_blueprint), calc_weight(0), trash_timer(0)
{
}

Object::~Object() { }

void Object::saveData(File::Writer& writer)
{
	// parent data
	Entity::saveData(writer);

	// save blueprint
	if (blueprint) {
		if (blueprint->isAnonymous()) {
			writer.begin("object", "blueprint");
			blueprint->save(writer);
			writer.end();
		} else {
			writer.attr("object", "blueprint", getBlueprint()->getId());
		}
	}

	// save name, if set
	if (!name.empty())
		writer.attr("object", "name", name.getFull());

	// parent location
	if (in_container == ObjectLocation::IN)
		writer.attr("object", "location", "in");
	if (in_container == ObjectLocation::ON)
		writer.attr("object", "location", "on");

	// save children objects
	for (EList<Object>::const_iterator e = children.begin(); e != children.end(); ++e) {
		(*e)->save(writer, "object", "child");
	}
}

void Object::saveHook(File::Writer& writer)
{
	Entity::saveHook(writer);
	Hooks::saveObject(this, writer);
}

int Object::loadFinish()
{
	if (blueprint == NULL) {
		Log::Error << "object has no blueprint";
		return -1;
	}

	recalcWeight();

	return 0;
}

int Object::loadNode(File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
	FO_OBJECT("object", "blueprint")
	ObjectBP* blueprint = new ObjectBP();
	if (blueprint->load(reader) != FO_SUCCESS_CODE) {
		delete blueprint;
		throw new File::Error("Failed to load unique object");
	} else {
		setBlueprint(blueprint);
	}
	FO_ATTR("object", "location")
	if (node.getString() == "in")
		in_container = ObjectLocation::IN;
	else if (node.getString() == "on")
		in_container = ObjectLocation::ON;
	else
		throw File::Error("Object has invalid container attribute");
	FO_ATTR("object", "blueprint")
	// sets a real blueprint
	ObjectBP* blueprint = NULL;
	if ((blueprint = MObjectBP.lookup(node.getString())) == NULL)
		Log::Error << "Could not find object blueprint '" << node.getString() << "'";
	else
		setBlueprint(blueprint);
	FO_ATTR("object", "name")
	name.setFull(node.getString());
	FO_ENTITY("object", "child")
	if (OBJECT(entity) == NULL) throw File::Error("Object child is not an Object");
	OBJECT(entity)->setOwner(this);
	children.add(OBJECT(entity));
	FO_PARENT(Entity)
	FO_NODE_END
}

void Object::setOwner(Entity* s_owner)
{
	// type check
	assert(OBJECT(s_owner) || ROOM(s_owner) || CHARACTER(s_owner));

	// set owner
	Entity::setOwner(s_owner);
	owner = s_owner;
}

void Object::ownerRelease(Entity* child)
{
	// we only own objects
	Object* obj = OBJECT(child);
	assert(obj != NULL);

	// find it
	EList<Object>::iterator e = std::find(children.begin(), children.end(), obj);
	if (e != children.end()) {
		obj->in_container = ObjectLocation::NONE;
		children.erase(e);
		return;
	}
}

void Object::heartbeat()
{
	// see if we can trash the object
	if (isTrashable()) {
		// must be laying in a room
		Room* room = ROOM(getOwner());
		if (room != NULL) {
			// rotting?
			if (isRotting() && trash_timer >= OBJECT_ROT_TICKS) {
				// destroy it
				*room << StreamName(this, INDEFINITE, true) << " rots away.\n";
				destroy();

				// not rotting - normal trash
			} else if (trash_timer >= OBJECT_TRASH_TICKS) {
				// room must not have any players in it
				if (room->countPlayers() == 0) {
					// destroy it
					destroy();
				}
			} else {
				++trash_timer;
			}
		}
	}

	// call update hook
	Hooks::objectHeartbeat(this);
}

void Object::activate()
{
	Entity::activate();

	for (EList<Object>::iterator e = children.begin(); e != children.end(); ++e)
		(*e)->activate();
}

void Object::deactivate()
{
	for (EList<Object>::iterator e = children.begin(); e != children.end(); ++e)
		(*e)->deactivate();

	Entity::deactivate();
}

bool Object::addObject(Object *object, ObjectLocation container)
{
	assert(object != NULL);

	// has contianer?
	if (!hasLocation(container))
		return false;

	// release and add
	object->setOwner(this);
	object->in_container = container;
	children.add(object);

	// recalc our weight, and parent's weight
	recalcWeight();
	if (OBJECT(owner))
		((Object*)owner)->recalcWeight();

	// ok add
	return true;
}

void Object::showContents(Player *player, ObjectLocation container) const
{
	*player << "You see ";

	Object* last = NULL;
	int displayed = 0;

	// show objects
	for (EList<Object>::const_iterator i = children.begin(); i != children.end(); ++i) {
		// not right container?
		if ((*i)->in_container != container)
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
	std::string tname = "somewhere on";
	if (container == ObjectLocation::ON)
		tname = "on";
	else if (container == ObjectLocation::IN)
		tname = "in";
	*player << " " << tname << " " << StreamName(*this, DEFINITE, false) << ".\n";
}

Object* Object::findObject(const std::string& name, uint index, ObjectLocation container, uint *matches) const
{
	assert(index != 0);

	// clear matches
	if (matches)
		*matches = 0;

	for (EList<Object>::const_iterator i = children.begin(); i != children.end(); ++i) {
		// right container container
		if ((*i)->in_container == container) {
			// check name
			if ((*i)->nameMatch(name)) {
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
void Object::recalcWeight()
{
	calc_weight = 0;

	// add up weight of objects
	for (EList<Object>::const_iterator i = children.begin(); i != children.end(); ++i)
		calc_weight += (*i)->getWeight();
}

// find parent room
Room* Object::getRoom() const
{
	Entity* owner = getOwner();
	while (owner != NULL && !ROOM(owner))
		owner = owner->getOwner();
	return ROOM(owner);
}

// find parent owner
Creature* Object::getHolder() const
{
	Entity* owner = getOwner();
	while (owner != NULL && OBJECT(owner))
		owner = owner->getOwner();
	return CHARACTER(owner);
}

bool Object::setName(const std::string& s_name)
{
	bool ret = name.setFull(s_name);
	return ret;
}

// get object name information
EntityName Object::getName() const
{
	assert(blueprint != NULL);
	if (name.empty())
		return blueprint->getName();
	else
		return name;
}

// get object description
std::string Object::getDesc() const
{
	assert(blueprint != NULL);
	return blueprint->getDesc();
}

// get object properties
uint Object::getCost() const
{
	assert(blueprint != NULL);
	return blueprint->getCost();
}

uint Object::getRealWeight() const
{
	assert(blueprint != NULL);
	return blueprint->getWeight();
}

EquipSlot Object::getEquip() const
{
	assert(blueprint != NULL);
	return blueprint->getEquip();
}

// get parsable member values
int Object::macroProperty(const StreamControl& stream, const std::string& comm, const MacroList& argv) const
{
	// COST
	if (strEq(comm, "cost")) {
		stream << getCost();
		return 0;
	}
	// WEIGHT
	if (strEq(comm, "weight")) {
		stream << getWeight();
		return 0;
	}
	// try the entity
	return Entity::macroProperty(stream, comm, argv);
}

// event handling
void Object::handleEvent(const Event& event)
{
	Entity::handleEvent(event);
}

void Object::broadcastEvent(const Event& event)
{
	for (EList<Object>::const_iterator i = children.begin(); i != children.end(); ++i)
		MEvent.resend(event, *i);
}

void Object::setBlueprint(ObjectBP* s_blueprint)
{
	if (blueprint != NULL && blueprint->isAnonymous())
		delete blueprint;
	blueprint = s_blueprint;
}

// load object from a blueprint
Object* Object::loadBlueprint(const std::string& name)
{
	ObjectBP* blueprint = MObjectBP.lookup(name);
	if (!blueprint)
		return NULL;

	return new Object(blueprint);
}

bool Object::isBlueprint(const std::string& name) const
{
	ObjectBP* blueprint = getBlueprint();

	if (blueprint != NULL)
		return strEq(blueprint->getId(), name);

	return false;
}

bool Object::nameMatch(const std::string& match) const
{
	if (getName().matches(match))
		return true;

	// blueprint keywords
	ObjectBP* blueprint = getBlueprint();
	if (blueprint != NULL) {
		for (std::vector<std::string>::const_iterator i = blueprint->getKeywords().begin(); i != blueprint->getKeywords().end(); ++i)
			if (phraseMatch(*i, match))
				return true;
	}

	// no match
	return false;
}

BEGIN_EFACTORY(Object)
return new Object();
END_EFACTORY
