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
	EventTimer (NameID set_Event, unsigned long when, Entity* set_target, Entity* set_trigger);
	~EventTimer (void);

	unsigned long get_when (void) const { return when; }
	NameID get_event (void) const { return event; }
	Entity* get_target (void) { return target; }
	const Entity* get_target (void) const { return target; }
	Entity* get_trigger (void) { return trigger; }
	const Entity* get_trigger (void) const { return trigger; }
};

namespace triggers
{
	int init (void);
	void close (void);

	set (NameID id, Entity* target, Entity* trigger);
	void update (void);
}

#endif
