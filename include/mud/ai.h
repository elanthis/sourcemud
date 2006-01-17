/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AI_H
#define AI_H

#include "common/awestr.h"
#include "common/imanager.h"
#include "common/gcbase.h"
#include "common/gcmap.h"
#include "mud/server.h"
#include "scriptix/function.h"

class Event;
class Character;
class ScriptRestrictedWriter;

class AI : public GC
{
	public:
	AI (StringArg s_name);

	int load (File::Reader& reader);

	void do_load (Character* self) const;
	void do_save (Character* self, ScriptRestrictedWriter* writer) const;
	void do_event (Character* self, const Event& event) const;
	void do_ready (Character* self) const;
	void do_heartbeat (Character* self) const;
	void do_pump (Character* self, Scriptix::Value arg) const;

	StringArg get_name (void) const { return name; }

	// data
	protected:
	typedef GCType::map<EventID, Scriptix::ScriptFunction> EventList;

	String name;

	EventList event_cb;

	Scriptix::ScriptFunction load_cb;
	Scriptix::ScriptFunction save_cb;
	Scriptix::ScriptFunction heartbeat_cb;
	Scriptix::ScriptFunction ready_cb;
	Scriptix::ScriptFunction pump_cb;

	friend class SAIManager;
};

class SAIManager : public IManager
{
	public:
	virtual int initialize (void);
	virtual void shutdown (void);

	void add (AI* ai);
	AI* get (StringArg name);

	private:
	typedef GCType::vector<AI*> AIList;
	AIList ai;
	
};
extern SAIManager AIManager;

#endif
