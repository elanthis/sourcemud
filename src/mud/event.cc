/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */


#include <stdlib.h>
#include <stdio.h>

#include "mud/entity.h"
#include "common/string.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/eventids.h"
#include "scriptix/native.h"

SEventManager EventManager;

SCRIPT_TYPE(EventHandler);
EventHandler::EventHandler (void) : Scriptix::Native(AweMUD_EventHandlerType), event(), script(), sxfunc() {}
EventHandler::EventHandler (EventID s_event) : Scriptix::Native(AweMUD_EventHandlerType), event(s_event), script(), sxfunc() {}

int
EventHandler::load (File::Reader& reader) {
	FO_READ_BEGIN
		FO_ATTR("id")
			event = EventID::create(node.get_name());
		FO_ATTR("script")
			sxfunc = NULL;
			if (node.get_data())
				sxfunc = EventManager.compile(event, node.get_data(), reader.get_filename(), node.get_line());
	FO_READ_ERROR
		return -1;
	FO_READ_END

	return 0;
}

void
EventHandler::save (File::Writer& writer) const {
	writer.attr(S("id"), EventID::nameof(event));
	if (script)
		writer.block(S("script"), script);
}

int
Entity::handle_event (const Event& event)
{
	for (EventList::const_iterator i = events.begin (); i != events.end (); ++ i)
		if (event.get_id() == (*i)->get_event ()) {
			if (!(*i)->get_func().empty())
				(*i)->get_func().run(this, event.get_id().name(), event.get_actor(), event.get_room(), event.get_data(0), event.get_data(1), event.get_data(2), event.get_data(3), event.get_data(4));
		}
	return 0;
}

int
SEventManager::initialize (void)
{
	count = 0;
	initialize_ids();

	return 0;
}

void
SEventManager::shutdown (void)
{
	events.clear();
}

int
SEventManager::send (EventID id, Room* room, Entity *actor, Scriptix::Value data1, Scriptix::Value data2, Scriptix::Value data3, Scriptix::Value data4, Scriptix::Value data5)
{
	assert (id.valid());

	// fetch room if we haev none given
	if (room == NULL) {
		Entity* ent = actor;
		while (ent != NULL && !ROOM(ent))
			ent = ent->get_owner();
		if (ent == NULL)
			return -1;
		room = (Room*)ent;
	}

	// create event
	Event event(id, room, actor);
	event.get_data(0) = data1;
	event.get_data(1) = data2;
	event.get_data(2) = data3;
	event.get_data(3) = data4;
	event.get_data(4) = data5;
	events.push_back(event);
	++count;

	return 0;
}

void
SEventManager::process (void)
{
	// remember how many events we started with
	// this way, events which trigger more events
	// won't cause an infinite loop, because we'll
	// only process the initial batch
	size_t processing = count;

	// as long as we have events...
	Event event;
	while (processing >= 0 && !events.empty()) {
		// get event
		event = events.front();
		events.pop_front();
		--count;
		--processing;

		// DEBUG:
		/*
		Log::Info << "Event[" << event.get_name() <<
			"] P:" << event.get_id().name() <<
			" R:" << (event.get_room() ? event.get_room()->get_id().c_str() : "n/a") <<
			" A:" << (event.get_actor() ? event.get_actor()->get_name().get_name().c_str() : "n/a") <<
			" D:" << (event.get_data(0) ? event.get_data(0).get_type()->get_name().name().c_str() : "n/a") <<
			" D:" << (event.get_data(1) ? event.get_data(1).get_type()->get_name().name().c_str() : "n/a") <<
			" D:" << (event.get_data(2) ? event.get_data(2).get_type()->get_name().name().c_str() : "n/a") <<
			" D:" << (event.get_data(3) ? event.get_data(3).get_type()->get_name().name().c_str() : "n/a") <<
			" D:" << (event.get_data(4) ? event.get_data(4).get_type()->get_name().name().c_str() : "n/a");
		*/

		// send event
		event.get_room()->handle_event(event);
	}
}
