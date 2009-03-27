/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_BODY_H
#define SOURCEMUD_MUD_BODY_H

#include "mud/fileobj.h"
#include "mud/server.h"

// Gender
class GenderType
{
public:
	typedef enum {
		NONE = 0,
		FEMALE,
		MALE,
		COUNT
	} type_t;

public:
	inline GenderType(int s_value) : value((type_t)s_value) {}
	inline GenderType() : value(NONE) {}

	inline const std::string& getName() const { return names[value]; }

	inline const std::string& getHisHer() const { return hisher[value]; }
	inline const std::string& getHisHers() const { return hishers[value]; }
	inline const std::string& getHeShe() const { return heshe[value]; }
	inline const std::string& getHimHer() const { return himher[value]; }
	inline const std::string& getManWoman() const { return manwoman[value]; }
	inline const std::string& getMaleFemale() const { return malefemale[value]; }

	inline type_t getValue() const { return value; }

	static GenderType lookup(const std::string& name);

	inline bool operator == (const GenderType& gender) const { return gender.value == value; }
	inline bool operator != (const GenderType& gender) const { return gender.value != value; }

private:
	type_t value;

	static std::string names[];
	static std::string hisher[];
	static std::string hishers[];
	static std::string heshe[];
	static std::string himher[];
	static std::string manwoman[];
	static std::string malefemale[];
};

// Equip slot types
class EquipSlot
{
public:
	typedef enum {
		NONE = 0,
		HEAD,
		TORSO,
		ARMS,
		LEGS,
		HANDS,
		FEET,
		NECK,
		BODY,
		BACK,
		WAIST,
		COUNT
	} type_t;

public:
	EquipSlot(int s_value) : value((type_t)s_value) {}
	EquipSlot() : value(NONE) {}

	bool valid() const { return value != NONE; }

	std::string getName() const { return names[value]; }

	type_t getValue() const { return value; }

	static EquipSlot lookup(const std::string& name);

	inline bool operator == (const EquipSlot& dir) const { return dir.value == value; }
	inline bool operator != (const EquipSlot& dir) const { return dir.value != value; }

private:
	type_t value;

	static std::string names[];
};

#endif
