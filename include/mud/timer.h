/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef TIMER_H
#define TIMER_H

#include "scriptix.h"

class EventTimer
{
private:
	NameID event;
	Entity* target;
	Entity* trigger;
	unsigned long when;

public:
	EventTimer(NameID set_Event, unsigned long when, Entity* set_target, Entity* set_trigger);
	~EventTimer();

	unsigned long get_when() const { return when; }
	NameID get_event() const { return event; }
	Entity* get_target() { return target; }
	const Entity* get_target() const { return target; }
	Entity* get_trigger() { return trigger; }
	const Entity* get_trigger() const { return trigger; }
};

namespace triggers
{
int init();
void close();

set(NameID id, Entity* target, Entity* trigger);
void update();
}

#endif
