/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_CONTAINER_H
#define AWEMUD_MUD_CONTAINER_H

#include "common/awestr.h"

// Container type object
class ContainerType {
	public:
	typedef enum {
		NONE = 0,
		IN,
		ON,
		UNDER,
		BEHIND,
		COUNT
	} type_t;
	
	public:
	ContainerType (int s_value) : value((type_t)s_value) {}
	ContainerType (void) : value(NONE) {}

	bool valid (void) const { return value != NONE; }

	StringArg get_name(void) const { return names[value]; }

	type_t get_value (void) const { return value; }

	static ContainerType lookup (StringArg name);

	inline bool operator == (const ContainerType& type) const { return type.value == value; }
	inline bool operator != (const ContainerType& type) const { return type.value != value; }
	inline bool operator < (const ContainerType& type) const { return value < type.value; }

	private:
	type_t value;

	static String names[];
};

#endif
