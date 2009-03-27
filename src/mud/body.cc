/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/error.h"
#include "common/string.h"
#include "common/streams.h"
#include "mud/body.h"
#include "mud/object.h"
#include "mud/creature.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/settings.h"

std::string GenderType::names[GenderType::COUNT] = {
	"none",
	"female",
	"male",
};
std::string GenderType::hisher[GenderType::COUNT] = {
	"its",
	"her",
	"his",
};
std::string GenderType::hishers[GenderType::COUNT] = {
	"its",
	"hers",
	"his",
};
std::string GenderType::heshe[GenderType::COUNT] = {
	"it",
	"she",
	"he",
};
std::string GenderType::himher[GenderType::COUNT] = {
	"it",
	"her",
	"him",
};
std::string GenderType::manwoman[GenderType::COUNT] = {
	"thing",
	"woman",
	"man",
};
std::string GenderType::malefemale[GenderType::COUNT] = {
	"neuter",
	"female",
	"male",
};

GenderType GenderType::lookup(const std::string& name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (strEq(name, names[i]))
			return i;
	return NONE;
}

std::string EquipSlot::names[] = {
	"none",
	"head",
	"torso",
	"arm",
	"leg",
	"hand",
	"foot",
	"neck",
	"body",
	"back",
	"waist"
};

EquipSlot EquipSlot::lookup(const std::string& name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (strEq(name, names[i]))
			return i;
	return NONE;
}

bool Creature::isHeld(Object *obj) const
{
	assert(obj != NULL);

	if (equipment.right_held == obj)
		return true;
	if (equipment.left_held == obj)
		return true;
	return false;
}

bool Creature::isWorn(Object *obj) const
{
	assert(obj != NULL);

	if (equipment.body_worn == obj)
		return true;
	if (equipment.waist_worn == obj)
		return true;
	if (equipment.back_worn == obj)
		return true;

	return false;
}

bool Creature::isEquipped(Object *obj) const
{
	assert(obj != NULL);
	return isHeld(obj) || isWorn(obj);
}

int Creature::hold(Object *obj)
{
	assert(obj != NULL);

	if (equipment.right_held == NULL) {
		obj->setOwner(this);
		equipment.right_held = obj;
		return 0;
	}
	if (equipment.left_held == NULL) {
		obj->setOwner(this);
		equipment.left_held = obj;
		return 1;
	}
	return -1;
}

int Creature::wear(Object *obj)
{
	assert(obj != NULL);

	if (equipment.body_worn == NULL && obj->getEquip() == EquipSlot::TORSO) {
		obj->setOwner(this);
		equipment.body_worn = obj;
		return 2;
	}
	if (equipment.back_worn == NULL && obj->getEquip() == EquipSlot::BACK) {
		obj->setOwner(this);
		equipment.back_worn = obj;
		return 3;
	}
	if (equipment.waist_worn == NULL && obj->getEquip() == EquipSlot::WAIST) {
		obj->setOwner(this);
		equipment.waist_worn = obj;
		return 4;
	}

	return -1;
}

int Creature::equip(Object *obj)
{
	assert(obj != NULL);

	int ret = wear(obj);
	if (ret >= 0)
		return ret;

	return hold(obj);
}

void Creature::releaseObject(Object *obj)
{
	assert(obj != NULL);

	if (equipment.right_held == obj)
		equipment.right_held = NULL;
	else if (equipment.left_held == obj)
		equipment.left_held = NULL;
	else if (equipment.body_worn == obj)
		equipment.body_worn = NULL;
	else if (equipment.back_worn == obj)
		equipment.back_worn = NULL;
	else if (equipment.waist_worn == obj)
		equipment.waist_worn = NULL;
}

int Creature::freeHands() const
{
	int hands = 0;
	if (equipment.right_held == NULL)
		hands ++;
	if (equipment.left_held == NULL)
		hands ++;
	return hands;
}

Object* Creature::getHeldAt(uint i) const
{
	if (equipment.right_held) {
		if (!i)
			return equipment.right_held;
		-- i;
	}

	if (equipment.left_held) {
		if (!i)
			return equipment.left_held;
		-- i;
	}

	return NULL;
}

Object* Creature::getWornAt(uint i) const
{
	if (equipment.body_worn) {
		if (!i)
			return equipment.body_worn;
		-- i;
	}
	if (equipment.back_worn) {
		if (!i)
			return equipment.back_worn;
		-- i;
	}
	if (equipment.waist_worn) {
		if (!i)
			return equipment.waist_worn;
		-- i;
	}

	return NULL;
}

Object* Creature::getEquipAt(uint i) const
{
	Object *ret;
	ret = getHeldAt(i);
	if (ret)
		return ret;
	if (equipment.right_held) i --;
	if (equipment.left_held) i --;

	return getWornAt(i);
}

Object* Creature::findWorn(const std::string& name, uint count, uint *matches) const
{
	assert(count != 0);

	// count
	if (matches)
		*matches = 0;

	if (equipment.body_worn != NULL && equipment.body_worn->nameMatch(name)) {
		if (matches)
			++ *matches;
		if (--count == 0)
			return equipment.body_worn;
	}
	if (equipment.back_worn != NULL && equipment.back_worn->nameMatch(name)) {
		if (matches)
			++ *matches;
		if (--count == 0)
			return equipment.back_worn;
	}
	if (equipment.waist_worn != NULL && equipment.waist_worn->nameMatch(name)) {
		if (matches)
			++ *matches;
		if (--count == 0)
			return equipment.waist_worn;
	}

	return NULL;
}

Object* Creature::findHeld(const std::string& name, uint count, uint *matches) const
{
	assert(count != 0);

	// count
	if (matches)
		*matches = 0;

	if (equipment.right_held != NULL && equipment.right_held->nameMatch(name)) {
		if (matches)
			++ *matches;
		if (--count == 0)
			return equipment.right_held;
	}

	if (equipment.left_held != NULL && equipment.left_held->nameMatch(name)) {
		if (matches)
			++ *matches;
		if (--count == 0)
			return equipment.left_held;
	}

	return NULL;
}

Object* Creature::findEquip(const std::string& name, uint count, uint *matches) const
{
	assert(count != 0);
	uint held_matches;

	Object *obj;
	if ((obj = findHeld(name, count, &held_matches)) != NULL) {
		if (matches)
			*matches = held_matches;
		return obj;
	} else {
		// update matches */
		count -= held_matches;
		// if we have a count left
		if (count) {
			obj = findWorn(name, count, matches);
			// total matches
			if (matches)
				*matches += held_matches;
			return obj;
		} else {
			// set matches
			if (matches)
				*matches = held_matches;
		}
	}

	return NULL;
}

void Creature::swapHands()
{
	Object *temp = equipment.right_held;
	equipment.right_held = equipment.left_held;
	equipment.left_held = temp;
}

void Creature::dropHeld(Room *r)
{
	assert(r != NULL);

	if (equipment.right_held && equipment.right_held->isDropable()) {
		r->addObject(equipment.right_held);
		equipment.right_held = NULL;
	}
	if (equipment.left_held && equipment.left_held->isDropable()) {
		r->addObject(equipment.left_held);
		equipment.left_held = NULL;
	}
}

void Creature::dropAll(Room *r)
{
	assert(r != NULL);

	dropHeld(r);

	if (equipment.body_worn && equipment.body_worn->isDropable()) {
		r->addObject(equipment.body_worn);
		equipment.body_worn = NULL;
	}
	if (equipment.back_worn && equipment.back_worn->isDropable()) {
		r->addObject(equipment.back_worn);
		equipment.back_worn = NULL;
	}
	if (equipment.waist_worn && equipment.waist_worn->isDropable()) {
		r->addObject(equipment.waist_worn);
		equipment.waist_worn = NULL;
	}
}

void Creature::ownerRelease(Entity* child)
{
	// we only hold objects
	Object* obj = OBJECT(child);
	assert(obj != NULL);

	// find and remove
	if (equipment.right_held == obj) {
		equipment.right_held = NULL;
		return;
	}
	if (equipment.left_held == obj) {
		equipment.left_held = NULL;
		return;
	}
	if (equipment.body_worn == obj) {
		equipment.body_worn = NULL;
		return;
	}
	if (equipment.back_worn == obj) {
		equipment.back_worn = NULL;
		return;
	}
	if (equipment.waist_worn == obj) {
		equipment.waist_worn = NULL;
		return;
	}
}
