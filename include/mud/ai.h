/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef AI_H
#define AI_H

#include "common/string.h"
#include "common/imanager.h"
#include "common/gcbase.h"
#include "common/gcmap.h"
#include "mud/server.h"

class Event;
class Creature;
class ScriptRestrictedWriter;

class AI : public GC
{
	public:
	AI () {}

	int load (File::Reader& reader);

	void do_load (Creature* self) const;
	void do_save (Creature* self, ScriptRestrictedWriter* writer) const;
	bool do_event (Creature* self, const Event& event) const;
	void do_ready (Creature* self) const;
	void do_heartbeat (Creature* self) const;
	void do_pump (Creature* self) const;

	String get_name (void) const { return name; }

	// data
	protected:
	typedef GCType::map<EventID, String> EventList;

	String name;

	EventList event_cb;

	String load_cb;
	String save_cb;
	String heartbeat_cb;
	String ready_cb;
	String pump_cb;

	friend class SAIManager;
};

class SAIManager : public IManager
{
	public:
	virtual int initialize (void);
	virtual void shutdown (void);

	void add (AI* ai);
	AI* get (String name);

	private:
	typedef GCType::vector<AI*> AIList;
	AIList ai;
	
};
extern SAIManager AIManager;

#endif
