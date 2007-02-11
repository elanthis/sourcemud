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

static const size_t EVENT_MAX_NEST = 10;

SCRIPT_TYPE(EventHandler);
EventHandler::EventHandler () : type(EVENT_NOTIFY), event(), script(), sxfunc() {}

int
EventHandler::load (File::Reader& reader) {
	FO_READ_BEGIN
		FO_ATTR("event", "type")
			if (node.get_string() == "request")
				type = EVENT_REQUEST;
			else if (node.get_string() == "notify")
				type = EVENT_NOTIFY;
			else if (node.get_string() == "command")
				type = EVENT_COMMAND;
			else {
				Log::Error << "Unknown event type '" << node.get_string() << "'";
				return -1;
			}
		FO_ATTR("event", "id")
			event = EventID::create(node.get_string());
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
	writer.attr(S("event"), S("type"), type == EVENT_REQUEST ? S("request") : type == EVENT_NOTIFY ? S("notify") : S("command"));
	writer.attr(S("event"), S("id"), EventID::nameof(event));
	if (script)
		writer.block(S("event"), S("script"), script);
}

bool
Entity::handle_event (const Event& event)
{
	Scriptix::Value argv[12] = { this, event.get_type(), event.get_id().name(), event.get_room(), event.get_actor(), event.get_target(), event.get_aux(), event.get_data(0), event.get_data(1), event.get_data(2), event.get_data(3), event.get_data(4) };
	for (EventList::const_iterator i = events.begin (); i != events.end (); ++ i)
		if (event.get_type() == (*i)->get_type() && event.get_id() == (*i)->get_event ()) {
			if (!(*i)->get_func().empty()) {
				bool res = (*i)->get_func().run(12, argv).is_true();
				if (event.get_type() != EVENT_NOTIFY && !res)
					return false;
			}
		}
	return true;
}

int
SEventManager::initialize (void)
{
	count = 0;
	nest = 0;
	initialize_ids();

	return 0;
}

void
SEventManager::shutdown (void)
{
	events.clear();
}

bool
SEventManager::request (EventID id, Room* room, Entity *actor, Entity* target, Entity* aux, Scriptix::Value data1, Scriptix::Value data2, Scriptix::Value data3, Scriptix::Value data4, Scriptix::Value data5)
{
	assert (id.valid());
	assert (room != NULL);
	assert (actor != NULL);

	// manage nesting
	if (nest == EVENT_MAX_NEST) {
		Log::Error << "Event system exceeded maximum nested events of " << EVENT_MAX_NEST << " sending request event " << id.name();
		return false;
	}

	// get zone
	Zone* zone = NULL;
	if (room)
		room->get_zone();

	// create event
	Event event(EVENT_REQUEST, id, room, actor, target, aux);
	event.get_data(0) = data1;
	event.get_data(1) = data2;
	event.get_data(2) = data3;
	event.get_data(3) = data4;
	event.get_data(4) = data5;

	// do processing
	++nest;
	if (actor && !actor->handle_event(event)) {
		--nest;
		return false;
	}
	if (target && !actor->handle_event(event)) {
		--nest;
		return false;
	}
	if (aux && !actor->handle_event(event)) {
		--nest;
		return false;
	}
	if (room && !actor->handle_event(event)) {
		--nest;
		return false;
	}
	if (zone && !actor->handle_event(event)) {
		--nest;
		return false;
	}
	--nest;
	return true;
}

bool
SEventManager::command (EventID id, Room* room, Entity *actor, Entity* target, Entity* aux, Scriptix::Value data1, Scriptix::Value data2, Scriptix::Value data3, Scriptix::Value data4, Scriptix::Value data5)
{
	assert (id.valid());
	assert (room != NULL);
	assert (actor != NULL);

	// manage nesting
	if (nest == EVENT_MAX_NEST) {
		Log::Error << "Event system exceeded maximum nested events of " << EVENT_MAX_NEST << " sending command event " << id.name();
		return false;
	}

	// create event
	Event event(EVENT_COMMAND, id, room, actor, target, aux);
	event.get_data(0) = data1;
	event.get_data(1) = data2;
	event.get_data(2) = data3;
	event.get_data(3) = data4;
	event.get_data(4) = data5;

	// do processing
	++nest;
	bool result = false;
	if (actor)
		result = actor->handle_event(event);
	--nest;
	return result;
}

void
SEventManager::notify (EventID id, Room* room, Entity *actor, Entity* target, Entity* aux, Scriptix::Value data1, Scriptix::Value data2, Scriptix::Value data3, Scriptix::Value data4, Scriptix::Value data5)
{
	assert (id.valid());
	assert (room != NULL);
	assert (actor != NULL);

	// create event
	Event event(EVENT_NOTIFY, id, room, actor, target, aux);
	event.get_data(0) = data1;
	event.get_data(1) = data2;
	event.get_data(2) = data3;
	event.get_data(3) = data4;
	event.get_data(4) = data5;

	// just queue it, doesn't need to happen now
	events.push_back(event);
	++count;
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
	while (processing >= 0 && !events.empty()) {
		// get event
		Event event = events.front();
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
		if (event.get_room())
			event.get_room()->broadcast_event(event);
	}
}
