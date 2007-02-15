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
#include "generated/eventids.h"

/* external classes */
class Entity;
class Room;

class EventHandler : public GC {
	protected:
	EventID event;
	String script;
	Scriptix::ScriptFunction sxfunc;

	public:
	EventHandler ();

	int load (File::Reader& reader);
	void save (File::Writer& writer) const;

	EventID get_event () const { return event; }
	Scriptix::ScriptFunction get_func () const { return sxfunc; }
};

// should only be stack allocated
class Event : public GC
{
	public:
	EventID get_id () const { return id; }
	
	Entity* get_recipient () const { return recipient; }
	Entity* get_aux1 () const { return aux1; }
	Entity* get_aux2 () const { return aux2; }
	const Scriptix::Value& get_data1 () const { return data1; }
	const Scriptix::Value& get_data2 () const { return data2; }
	const Scriptix::Value& get_data3 () const { return data3; }
	const Scriptix::Value& get_data4 () const { return data4; }

	private:
	EventID id;
	Entity* recipient;
	Entity* aux1;
	Entity* aux2;
	Scriptix::Value data1;
	Scriptix::Value data2;
	Scriptix::Value data3;
	Scriptix::Value data4;

	friend class SEventManager;
};

class SEventManager : public IManager
{
	public:
	// initialize manager
	virtual int initialize ();

	// shutdown the manager
	virtual void shutdown ();

	void send (
		EventID id,
		Entity* recipient,
		Entity* aux1 = 0,
		Entity* aux2 = 0,
		Scriptix::Value data1 = Scriptix::Value(),
		Scriptix::Value data2 = Scriptix::Value(),
		Scriptix::Value data3 = Scriptix::Value(),
		Scriptix::Value data4 = Scriptix::Value()
	);

	void resend (const Event& event, Entity* recipient);

	void broadcast (
		EventID id,
		Entity* recipient,
		Entity* aux1 = 0,
		Entity* aux2 = 0,
		Scriptix::Value data1 = Scriptix::Value(),
		Scriptix::Value data2 = Scriptix::Value(),
		Scriptix::Value data3 = Scriptix::Value(),
		Scriptix::Value data4 = Scriptix::Value()
	);

	// return true if there are pending events
	bool events_pending () { return !events.empty(); }

	// compile an event handler script
	Scriptix::ScriptFunction compile (EventID id, String source, String filename, unsigned long fileline);

	// process events
	void process ();

	private:
	typedef std::deque<Event, gc_allocator<Event> > EQueue;
	EQueue events;
};
extern SEventManager EventManager;

#endif
