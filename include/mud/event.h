/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef EVENT_H 
#define EVENT_H

#include "mud/fileobj.h"

/* external classes */
class Room;
class Entity;
class Object;
class Portal;
class Zone;

class EventID {
	public:
	typedef enum {
		NONE = 0,
		LOOK,
		LEAVE_ROOM,
		ENTER_ROOM,
		LEAVE_ZONE,
		ENTER_ZONE,
		TOUCH_ITEM,
		GRASP_ITEM,
		RELEASE_ITEM,
		GET_ITEM,
		PUT_ITEM,
		DROP_ITEM,
		PICKUP_ITEM,
		COUNT,
	} type_t;
	
	explicit EventID (int s_value) : value((type_t)s_value) {}
	EventID (type_t s_value) : value(s_value) {}
	EventID (const EventID& event) : value(event.value) {}
	EventID () : value(NONE) {}

	bool valid () const { return value != NONE; }
	const std::string& get_name() const { return names[value]; }
	type_t get_value () const { return value; }

	static EventID lookup(const std::string& name);

	bool operator == (EventID dir) const { return dir.value == value; }
	bool operator < (EventID dir) const { return value < dir.value; }

	private:
	type_t value;

	static std::string names[];
};

class EventHandler {
	protected:
	EventID event;
	std::string script;

	public:
	EventHandler ();

	int load(File::Reader& reader);
	void save(File::Writer& writer) const;

	EventID get_event() const { return event; }
};

// should only be stack allocated
class Event
{
	public:
	EventID get_id() const { return id; }
	
	Entity* get_recipient() const { return recipient; }
	Entity* get_aux1() const { return aux1; }
	Entity* get_aux2() const { return aux2; }

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
	virtual int initialize();

	// shutdown the manager
	virtual void shutdown();

	void send (
		EventID id,
		Entity* recipient,
		Entity* aux1 = 0,
		Entity* aux2 = 0,
		Entity* aux3 = 0
	);

	void broadcast (
		EventID id,
		Entity* recipient,
		Entity* aux1 = 0,
		Entity* aux2 = 0,
		Entity* aux3 = 0
	);

	void resend(const Event& event, Entity* recipient);

	// return true if there are pending events
	bool events_pending() { return !events.empty(); }

	// compile an event handler script
	int compile(EventID id, const std::string& source, const std::string& filename, unsigned long fileline);

	// process events
	void process();

	private:
	typedef std::deque<Event> EQueue;
	EQueue events;
};
extern _MEvent MEvent;

namespace Events {
	void sendLook(Room* room, Creature* actor, Entity* target);
	void sendLeaveRoom(Room* room, Creature* actor, Portal* aux, Room* dest);
	void sendEnterRoom(Room* room, Creature* actor, Portal* aux, Room* from);
	void sendLeaveZone(Room* room, Creature* actor, Zone* dest);
	void sendEnterZone(Room* room, Creature* actor, Zone* from);
	void sendTouchItem(Room* room, Creature* actor, Object* target);
	void sendGraspItem(Room* room, Creature* actor, Object* target);
	void sendReleaseItem(Room* room, Creature* actor, Object* target);
	void sendGetItem(Room* room, Creature* actor, Object* target, Object* aux, const std::string& );
	void sendPutItem(Room* room, Creature* actor, Object* target, Object* aux, const std::string& );
	void sendDropItem(Room* room, Creature* actor, Object* target);
	void sendPickupItem(Room* room, Creature* actor, Object* target);
} // namespace Events

#endif
