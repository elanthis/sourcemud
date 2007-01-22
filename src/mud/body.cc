/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */


#include <stdlib.h>

// STL stuffs
#include <algorithm>

#include "mud/body.h"
#include "mud/object.h"
#include "common/error.h"
#include "common/string.h"
#include "mud/creature.h"
#include "mud/server.h"
#include "mud/room.h"
#include "common/streams.h"
#include "mud/settings.h"

String GenderType::names[GenderType::COUNT] = {
	S("none"),
	S("female"),
	S("male"),
};
String GenderType::hisher[GenderType::COUNT] = {
	S("its"),
	S("her"),
	S("his"),
};
String GenderType::hishers[GenderType::COUNT] = {
	S("its"),
	S("hers"),
	S("his"),
};
String GenderType::heshe[GenderType::COUNT] = {
	S("it"),
	S("she"),
	S("he"),
};
String GenderType::himher[GenderType::COUNT] = {
	S("it"),
	S("her"),
	S("him"),
};
String GenderType::manwoman[GenderType::COUNT] = {
	S("thing"),
	S("woman"),
	S("man"),
};
String GenderType::malefemale[GenderType::COUNT] = {
	S("neuter"),
	S("female"),
	S("male"),
};

GenderType
GenderType::lookup (String name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (str_eq(name, names[i]))
			return i;
	return NONE;
}

String EquipSlot::names[] = {
	S("none"),
	S("head"),
	S("torso"),
	S("arm"),
	S("leg"),
	S("hand"),
	S("foot"),
	S("neck"),
	S("body"),
	S("back"),
	S("waist")
};

EquipSlot
EquipSlot::lookup (String name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (str_eq(name, names[i]))
			return i;
	return NONE;
}

bool
Creature::is_held (Object *obj) const
{
	assert (obj != NULL);

	if (equipment.right_held == obj)
		return true;
	if (equipment.left_held == obj)
		return true;
	return false;
}

bool
Creature::is_worn (Object *obj) const
{
	assert (obj != NULL);

	if (equipment.body_worn == obj)
		return true;
	if (equipment.waist_worn == obj)
		return true;
	if (equipment.back_worn == obj)
		return true;

	return false;
}

bool
Creature::is_equipped (Object *obj) const
{
	assert (obj != NULL);
	return is_held (obj) || is_worn (obj);
}

int
Creature::hold (Object *obj)
{
	assert (obj != NULL);

	if (equipment.right_held == NULL) {
		obj->set_owner(this);
		equipment.right_held = obj;
		return 0;
	}
	if (equipment.left_held == NULL) {
		obj->set_owner(this);
		equipment.left_held = obj;
		return 1;
	}
	return -1;
}

int
Creature::wear (Object *obj) {
	assert (obj != NULL);

	if (equipment.body_worn == NULL && obj->get_equip () == EquipSlot::TORSO) {
		obj->set_owner(this);
		equipment.body_worn = obj;
		return 2;
	}
	if (equipment.back_worn == NULL && obj->get_equip () == EquipSlot::BACK) {
		obj->set_owner(this);
		equipment.back_worn = obj;
		return 3;
	}
	if (equipment.waist_worn == NULL && obj->get_equip () == EquipSlot::WAIST) {
		obj->set_owner(this);
		equipment.waist_worn = obj;
		return 4;
	}

	return -1;
}

int
Creature::equip (Object *obj) {
	assert (obj != NULL);

	int ret = wear (obj);
	if (ret >= 0)
		return ret;

	return hold (obj);
}

void
Creature::release_object (Object *obj)
{
	assert (obj != NULL);

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

int
Creature::free_hands (void) const
{
	int hands = 0;
	if (equipment.right_held == NULL)
		hands ++;
	if (equipment.left_held == NULL)
		hands ++;
	return hands;
}

Object *
Creature::get_held_at (uint i) const
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

Object *
Creature::get_worn_at (uint i) const
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

Object *
Creature::get_equip_at (uint i) const
{
	Object *ret;
	ret = get_held_at (i);
	if (ret)
		return ret;
	if (equipment.right_held) i --;
	if (equipment.left_held) i --;

	return get_worn_at (i);
}

Object *
Creature::find_worn (String name, uint count, uint *matches) const
{
	assert (count != 0);

	// count
	if (matches)
		*matches = 0;

	if (equipment.body_worn != NULL && equipment.body_worn->name_match (name)) {
		if (matches)
			++ *matches;
		if (--count == 0)
			return equipment.body_worn;
	}
	if (equipment.back_worn != NULL && equipment.back_worn->name_match (name)) {
		if (matches)
			++ *matches;
		if (--count == 0)
			return equipment.back_worn;
	}
	if (equipment.waist_worn != NULL && equipment.waist_worn->name_match (name)) {
		if (matches)
			++ *matches;
		if (--count == 0)
			return equipment.waist_worn;
	}

	return NULL;
}

Object *
Creature::find_held (String name, uint count, uint *matches) const
{
	assert (count != 0);

	// count
	if (matches)
		*matches = 0;

	if (equipment.right_held != NULL && equipment.right_held->name_match (name)) {
		if (matches)
			++ *matches;
		if (--count == 0)
			return equipment.right_held;
	}

	if (equipment.left_held != NULL && equipment.left_held->name_match (name)) {
		if (matches)
			++ *matches;
		if (--count == 0)
			return equipment.left_held;
	}

	return NULL;
}

Object *
Creature::find_equip (String name, uint count, uint *matches) const
{
	assert (count != 0);
	uint held_matches;

	Object *obj;
	if ((obj = find_held (name, count, &held_matches)) != NULL) {
		if (matches)
			*matches = held_matches;
		return obj;
	} else {
		// update matches */
		count -= held_matches;
		// if we have a count left
		if (count) {
			obj = find_worn (name, count, matches);
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

void
Creature::swap_hands (void) {
	Object *temp = equipment.right_held;
	equipment.right_held = equipment.left_held;
	equipment.left_held = temp;
}

void
Creature::drop_held (Room *r) {
	assert (r != NULL);

	if (equipment.right_held && equipment.right_held->is_dropable()) {
		r->add_object (equipment.right_held);
		equipment.right_held = NULL;
	}
	if (equipment.left_held && equipment.left_held->is_dropable()) {
		r->add_object (equipment.left_held);
		equipment.left_held = NULL;
	}
}

void
Creature::drop_all (Room *r) {
	assert (r != NULL);

	drop_held (r);

	if (equipment.body_worn && equipment.body_worn->is_dropable()) {
		r->add_object (equipment.body_worn);
		equipment.body_worn = NULL;
	}
	if (equipment.back_worn && equipment.back_worn->is_dropable()) {
		r->add_object (equipment.back_worn);
		equipment.back_worn = NULL;
	}
	if (equipment.waist_worn && equipment.waist_worn->is_dropable()) {
		r->add_object (equipment.waist_worn);
		equipment.waist_worn = NULL;
	}
}

void
Creature::owner_release (Entity* child)
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
