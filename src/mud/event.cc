/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "mud/entity.h"
#include "mud/server.h"
#include "mud/room.h"

_MEvent MEvent;

std::string EventID::names[] = {
	"None",
	"Look",
	"LeaveRoom",
	"EnterRoom",
	"LeaveZone",
	"EnterZone",
	"TouchItem",
	"GraspItem",
	"ReleaseItem",
	"GetItem",
	"PutItem",
	"DropItem",
	"PickupItem",
};

EventID EventID::lookup(const std::string& name)
{
	for (size_t i = 0; i < COUNT; ++i)
		if (name == names[i])
			return EventID(i);
	return EventID();
}

EventHandler::EventHandler() : event(), script() {}

int EventHandler::load(File::Reader& reader)
{
	FO_READ_BEGIN
	FO_ATTR("event", "id")
	event = EventID::lookup(node.getString());
	if (!event.valid())
		Log::Warning << node << ": Unknown event '" << node.getString() << "'";
	FO_ATTR("event", "script")
	script = node.getString();
	FO_READ_ERROR
	return -1;
	FO_READ_END

	return 0;
}

void EventHandler::save(File::Writer& writer) const
{
	writer.attr("event", "id", event.getName());
	if (!script.empty())
		writer.block("event", "script", script);
}

void Entity::handleEvent(const Event& event)
{
	for (EventList::const_iterator i = events.begin(); i != events.end(); ++ i) {
		if (event.getId() == (*i)->getEvent()) {
			//(*i)->getFunc().run(len, argv);
		}
	}
}

int _MEvent::initialize()
{
	return 0;
}

void _MEvent::shutdown()
{
	events.clear();
}

void _MEvent::send(EventID id, Entity* recipient, Entity* aux1, Entity* aux2, Entity* aux3)
{
	assert(id.valid());
	assert(recipient != NULL);

	// create event
	Event event;
	event.id = id;
	event.recipient = recipient;
	event.aux1 = aux1;
	event.aux2 = aux2;
	event.aux3 = aux3;

	// just queue it, doesn't need to happen now
	events.push_back(event);
}

void _MEvent::broadcast(EventID id, Entity* recipient, Entity* aux1, Entity* aux2, Entity* aux3)
{
	assert(id.valid());
	assert(recipient != NULL);

	// create event
	Event event;
	event.id = id;
	event.recipient = recipient;
	event.aux1 = aux1;
	event.aux2 = aux2;
	event.aux3 = aux3;

	// ask entity to broadcast it
	recipient->broadcastEvent(event);
}

void _MEvent::resend(const Event& event, Entity* recipient)
{
	Event new_event = event;
	new_event.recipient = recipient;
	events.push_back(new_event);
}

void _MEvent::process()
{
	// remember how many events we started with
	// this way, events which trigger more events
	// won't cause an infinite loop, because we'll
	// only process the initial batch
	size_t processing = events.size();

	// as long as we have events...
	while (processing > 0 && !events.empty()) {
		// get event
		Event event = events.front();
		events.pop_front();
		--processing;

		// DEBUG:
		/*
		Log::Info << "Event[" << event.getName() <<
			"] P:" << event.getId().name() <<
			" R:" << (event.getRoom() ? event.getRoom()->getId().c_str() : "n/a") <<
			" A:" << (event.getActor() ? event.getActor()->getName().getName().c_str() : "n/a") <<
			" D:" << (event.getData(0) ? event.getData(0).getType()->getName().name().c_str() : "n/a") <<
			" D:" << (event.getData(1) ? event.getData(1).getType()->getName().name().c_str() : "n/a") <<
			" D:" << (event.getData(2) ? event.getData(2).getType()->getName().name().c_str() : "n/a") <<
			" D:" << (event.getData(3) ? event.getData(3).getType()->getName().name().c_str() : "n/a") <<
			" D:" << (event.getData(4) ? event.getData(4).getType()->getName().name().c_str() : "n/a");
		*/

		// send event
		event.getRecipient()->handleEvent(event);
	}
}

int _MEvent::compile(EventID id, const std::string& source, const std::string& filename, unsigned long fileline)
{
	return 0;
}

namespace Events
{
	void sendLook(Room* room, Creature* actor, Entity* target)
	{
		/*EventManager.send(EventID::Look,room,actor,target,aux);*/
	}

	void sendLeaveRoom(Room* room, Creature* actor, Portal* aux, Room* arg_dest)
	{
		/*EventManager.send(EventID::LeaveRoom,room,actor,target,aux,arg_dest);*/
	}

	void sendEnterRoom(Room* room, Creature* actor, Portal* aux, Room* arg_from)
	{
		/*EventManager.send(EventID::EnterRoom,room,actor,target,aux,arg_from);*/
	}

	void sendLeaveZone(Room* room, Creature* actor, Zone* arg_dest)
	{
		/*EventManager.send(EventID::LeaveZone,room,actor,target,aux,arg_dest);*/
	}

	void sendEnterZone(Room* room, Creature* actor, Zone* arg_from)
	{
		/*EventManager.send(EventID::EnterZone,room,actor,target,aux,arg_from);*/
	}

	void sendTouchItem(Room* room, Creature* actor, Object* target)
	{
		/*EventManager.send(EventID::TouchItem,room,actor,target,aux);*/
	}

	void sendGraspItem(Room* room, Creature* actor, Object* target)
	{
		/*EventManager.send(EventID::GraspItem,room,actor,target,aux);*/
	}

	void sendReleaseItem(Room* room, Creature* actor, Object* target)
	{
		/*EventManager.send(EventID::ReleaseItem,room,actor,target,aux);*/
	}

	void sendGetItem(Room* room, Creature* actor, Object* target, Object* aux, const std::string& arg_)
	{
		/*EventManager.send(EventID::GetItem,room,actor,target,aux,arg_);*/
	}

	void sendPutItem(Room* room, Creature* actor, Object* target, Object* aux, const std::string& arg_)
	{
		/*EventManager.send(EventID::PutItem,room,actor,target,aux,arg_);*/
	}

	void sendDropItem(Room* room, Creature* actor, Object* target)
	{
		/*EventManager.send(EventID::DropItem,room,actor,target,aux);*/
	}

	void sendPickupItem(Room* room, Creature* actor, Object* target)
	{
		/*EventManager.send(EventID::PickupItem,room,actor,target,aux);*/
	}

} // namespace Events
