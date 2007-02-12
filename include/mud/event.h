/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef EVENT_H 
#define EVENT_H

#include <deque>

#include "mud/fileobj.h"
#include "common/imanager.h"
#include "scriptix/native.h"
#include "scriptix/function.h"
#include "mud/eventids.h"

/* external classes */
class Entity;
class Room;

enum EventType {
	EVENT_REQUEST,
	EVENT_NOTIFY,
	EVENT_COMMAND
};


class EventHandler : public GC {
	protected:
	EventType type;
	EventID event;
	String script;
	Scriptix::ScriptFunction sxfunc;

	public:
	EventHandler ();

	int load (File::Reader& reader);
	void save (File::Writer& writer) const;

	EventType get_type () const { return type; }
	EventID get_event () const { return event; }
	Scriptix::ScriptFunction get_func () const { return sxfunc; }
};

// should only be stack allocated
class Event : public GC
{
	public:
	Event (EventType s_type, EventID s_id, Room* s_room, Entity* s_actor, Entity* s_target, Entity* s_aux) : type(s_type), id(s_id), room(s_room), actor(s_actor), target(s_target), aux(s_target) {}
	Event (const Event& event) : id(event.id), room(event.room), actor(event.actor), target(event.target), aux(event.aux), data(event.data) {}

	EventType get_type () const { return type; }
	EventID get_id () const { return id; }
	String get_name () const { return id.get_name(); }
	
	Room* get_room () const { return room; }
	Entity* get_actor () const { return actor; }
	Entity* get_target () const { return target; }
	Entity* get_aux () const { return aux; }
	const Scriptix::Value& get_data (uint i) const { return data[i]; }
	Scriptix::Value& get_data (uint i) { return data[i]; }

	private:
	EventType type;
	EventID id;
	Room* room; // room the event occured in
	Entity* actor; // who/what initiated the event
	Entity* target; // target of the action (if any)
	Entity* aux; // auxillary entity (if any)
	Scriptix::Value data[5]; // misc data
};

class SEventManager : public IManager
{
	public:
	// initialize manager
	virtual int initialize ();

	// shutdown the manager
	virtual void shutdown ();

	bool request (EventID id, class Room* room, class Entity* actor, class Entity* target, class Entity* aux, Scriptix::Value data1 = Scriptix::Value(), Scriptix::Value data2 = Scriptix::Value(), Scriptix::Value data3 = Scriptix::Value(), Scriptix::Value data4 = Scriptix::Value(), Scriptix::Value data5 = Scriptix::Value());
	void notify (EventID id, class Room* room, class Entity* actor, class Entity* target, class Entity* aux, Scriptix::Value data1 = Scriptix::Value(), Scriptix::Value data2 = Scriptix::Value(), Scriptix::Value data3 = Scriptix::Value(), Scriptix::Value data4 = Scriptix::Value(), Scriptix::Value data5 = Scriptix::Value());
	bool command (EventID id, class Room* room, class Entity* actor, class Entity* target, class Entity* aux, Scriptix::Value data1 = Scriptix::Value(), Scriptix::Value data2 = Scriptix::Value(), Scriptix::Value data3 = Scriptix::Value(), Scriptix::Value data4 = Scriptix::Value(), Scriptix::Value data5 = Scriptix::Value());

	// return true if there are pending events
	bool events_pending () { return !events.empty(); }

	// compile an event handler script
	Scriptix::ScriptFunction compile (EventID id, String source, String filename, unsigned long fileline);

	// process events
	void process ();

	private:
	size_t nest;
	typedef std::deque<Event, gc_allocator<Event> > EQueue;
	EQueue events;
};
extern SEventManager EventManager;

#endif
