/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef EVENT_H 
#define EVENT_H

#include <deque>

#include "mud/fileobj.h"
#include "common/imanager.h"
#include "generated/events.h"

/* external classes */
class Entity;
class Room;

class EventHandler {
	protected:
	EventID event;
	std::string script;

	public:
	EventHandler ();

	int load (File::Reader& reader);
	void save (File::Writer& writer) const;

	EventID get_event () const { return event; }
};

// should only be stack allocated
class Event
{
	public:
	EventID get_id () const { return id; }
	
	Entity* get_recipient () const { return recipient; }
	Entity* get_aux1 () const { return aux1; }
	Entity* get_aux2 () const { return aux2; }

	private:
	EventID id;
	Entity* recipient;
	Entity* aux1;
	Entity* aux2;
	Entity* aux3;

	friend class _MEvent;
};

class _MEvent : public IManager
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
		Entity* aux3 = 0
	);

	void resend (const Event& event, Entity* recipient);

	void broadcast (
		EventID id,
		Entity* recipient,
		Entity* aux1 = 0,
		Entity* aux2 = 0,
		Entity* aux3 = 0
	);

	// return true if there are pending events
	bool events_pending () { return !events.empty(); }

	// compile an event handler script
	int compile (EventID id, std::string source, std::string filename, unsigned long fileline);

	// process events
	void process ();

	private:
	typedef std::deque<Event> EQueue;
	EQueue events;
};
extern _MEvent MEvent;

#endif
