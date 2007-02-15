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
#include "generated/eventids.h"
#include "scriptix/native.h"
#include "generated/bindings.h"

SEventManager EventManager;

SCRIPT_TYPE(EventHandler);
EventHandler::EventHandler () : event(), script(), sxfunc() {}

int
EventHandler::load (File::Reader& reader) {
	FO_READ_BEGIN
		FO_ATTR("event", "id")
			event = EventID::lookup(node.get_string());
			if (!event.valid()) 
				Log::Warning << node << ": Unknown event '" << node.get_string() << "'";
		FO_ATTR("event", "script")
			sxfunc = NULL;
			if (node.get_string())
				sxfunc = EventManager.compile(event, node.get_string(), reader.get_filename(), node.get_line());
	FO_READ_ERROR
		return -1;
	FO_READ_END

	return 0;
}

void
EventHandler::save (File::Writer& writer) const {
	writer.attr(S("event"), S("id"), event.get_name());
	if (script)
		writer.block(S("event"), S("script"), script);
}

void
Entity::handle_event (const Event& event)
{
	Scriptix::Value argv[] = {
		this,
		toValue(event.get_id()),
		event.get_recipient(),
		event.get_aux1(),
		event.get_aux2(),
		event.get_data1(),
		event.get_data2(),
		event.get_data3(),
		event.get_data4()
	};
	size_t len = sizeof(argv)/sizeof(argv[0]);

	for (EventList::const_iterator i = events.begin (); i != events.end (); ++ i) {
		if (event.get_id() == (*i)->get_event() && !(*i)->get_func().empty()) {
			(*i)->get_func().run(len, argv);
		}
	}
}

int
SEventManager::initialize ()
{
	return 0;
}

void
SEventManager::shutdown ()
{
	events.clear();
}

void
SEventManager::send (EventID id, Entity* recipient, Entity* aux1, Entity* aux2, Scriptix::Value data1, Scriptix::Value data2, Scriptix::Value data3, Scriptix::Value data4)
{
	assert (id.valid());
	assert (recipient != NULL);

	// create event
	Event event;
	event.id = id;
	event.recipient = recipient;
	event.aux1 = aux1;
	event.aux2 = aux2;
	event.data1 = data1;
	event.data2 = data2;
	event.data3 = data3;
	event.data4 = data4;

	// just queue it, doesn't need to happen now
	events.push_back(event);
}

void
SEventManager::broadcast (EventID id, Entity* recipient, Entity* aux1, Entity* aux2, Scriptix::Value data1, Scriptix::Value data2, Scriptix::Value data3, Scriptix::Value data4)
{
	assert (id.valid());
	assert (recipient != NULL);

	// create event
	Event event;
	event.id = id;
	event.recipient = recipient;
	event.aux1 = aux1;
	event.aux2 = aux2;
	event.data1 = data1;
	event.data2 = data2;
	event.data3 = data3;
	event.data4 = data4;

	// ask entity to broadcast it
	recipient->broadcast_event(event);
}

void
SEventManager::resend (const Event& event, Entity* recipient)
{
	Event new_event = event;
	new_event.recipient = recipient;
	events.push_back(new_event);
}

void
SEventManager::process ()
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
		event.get_recipient()->handle_event(event);
	}
}
