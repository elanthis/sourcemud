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
#include "mud/idmap.h"
#include "common/imanager.h"
#include "scriptix/native.h"
#include "scriptix/function.h"

/* external classes */
class Entity;
class Room;

DECLARE_IDMAP(Event)

class EventHandler : public Scriptix::Native {
	protected:
	EventID event;
	String script;
	Scriptix::ScriptFunction sxfunc;

	public:
	EventHandler (void);
	EventHandler (EventID s_event);

	int load (File::Reader& reader);
	void save (File::Writer& writer) const;

	EventID get_event (void) const { return event; }
	Scriptix::ScriptFunction get_func (void) const { return sxfunc; }
};

// should only be stack allocated
class Event : public GC
{
	public:
	inline Event (void) : id(), room(NULL), actor(NULL) {}
	inline Event (EventID s_id, Room* s_room, Entity* s_actor) : id(s_id), room(s_room), actor(s_actor) {}
	inline Event (const Event& event) : id(event.id), room(event.room), actor(event.actor), data(event.data) {}

	inline EventID get_id (void) const { return id; }
	inline String get_name (void) const { return EventID::nameof(id); }
	
	inline Room* get_room (void) const { return room; }
	inline Entity* get_actor (void) const { return actor; }
	inline const Scriptix::Value& get_data (uint i) const { return data[i]; }
	inline Scriptix::Value& get_data (uint i) { return data[i]; }

	private:
	EventID id;
	Room* room; // room the event occured in
	Entity* actor; // who/what initiated the event
	Scriptix::Value data[5]; // misc data
};

class SEventManager : public IManager
{
	public:
	// initialize manager
	virtual int initialize (void);

	// shutdown the manager
	virtual void shutdown (void);

	// send a quickie event
	int send (EventID id, class Room* room, class Entity* actor, Scriptix::Value data1 = Scriptix::Value(), Scriptix::Value data2 = Scriptix::Value(), Scriptix::Value data3 = Scriptix::Value(), Scriptix::Value data4 = Scriptix::Value(), Scriptix::Value data5 = Scriptix::Value());

	// return true if there are pending events
	inline bool events_pending (void) { return !events.empty(); }

	// compile an event handler script
	Scriptix::ScriptFunction compile (EventID id, String source, String filename, unsigned long fileline);

	// process events
	void process (void);

	private:
	size_t count;
	typedef std::deque<Event, gc_allocator<Event> > EQueue;
	EQueue events;

	void initialize_ids (void);
};
extern SEventManager EventManager;

#endif
