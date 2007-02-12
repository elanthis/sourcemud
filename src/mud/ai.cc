/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/settings.h"
#include "mud/creature.h"
#include "mud/ai.h"
#include "mud/room.h"
#include "mud/eventids.h"
#include "mud/bindings.h"
#include "common/manifest.h"

SAIManager AIManager;

int
AI::load (File::Reader& reader)
{
	FO_READ_BEGIN
		FO_ATTR("ai", "id")
			name = node.get_string();
		FO_ATTR("event", "load")
			load_cb = Scriptix::ScriptFunction::compile(S("load"), node.get_string(), S("self"), reader.get_filename(), node.get_line());
			
			if (load_cb.empty())
				throw File::Error(S("Script compile failed"));
		FO_ATTR("event", "save")
			save_cb = Scriptix::ScriptFunction::compile(S("save"), node.get_string(), S("self,writer"), reader.get_filename(), node.get_line());
			
			if (save_cb.empty())
				throw File::Error(S("Script compile failed"));
		FO_ATTR("event", "heartbeat")
			heartbeat_cb = Scriptix::ScriptFunction::compile(S("heartbeat"), node.get_string(), S("self"), reader.get_filename(), node.get_line());
			
			if (heartbeat_cb.empty())
				throw File::Error(S("Script compile failed"));
		FO_ATTR("event", "ready")
			ready_cb = Scriptix::ScriptFunction::compile(S("ready"), node.get_string(), S("self"), reader.get_filename(), node.get_line());
			
			if (ready_cb.empty())
				throw File::Error(S("Script compile failed"));
		FO_ATTR("event", "pump")
			pump_cb = Scriptix::ScriptFunction::compile(S("pump"), node.get_string(), S("self,data"), reader.get_filename(), node.get_line());
			
			if (pump_cb.empty())
				throw File::Error(S("Script compile failed"));
		FO_WILD("event")
			EventID event = EventID::lookup(node.get_name());
			if (event.valid()) {
				Scriptix::ScriptFunction handler = EventManager.compile(event, node.get_string(), reader.get_filename(), node.get_line());

				if (handler.empty())
					throw File::Error(S("Script compile failed"));

				event_cb[EventID::lookup(node.get_name())] = handler;
			}
	FO_READ_ERROR
		return -1;
	FO_READ_END

	if (name.empty()) {
		Log::Error << "AI in " << reader.get_path() << " has no ID.";
		return -1;
	}

	return 0;
}

bool
AI::do_event (Creature* self, const Event& event) const
{
	// get handler
	EventList::const_iterator i = event_cb.find(event.get_id());
	if (i != event_cb.end()) {
		bool res = i->second.run(self, event.get_id().get_name(), event.get_actor(), event.get_room(), event.get_data(0), event.get_data(1), event.get_data(2), event.get_data(3), event.get_data(4)).is_true();
		if (event.get_type() != EVENT_NOTIFY && !res)
			return false;
	}
	return true;
}

void
AI::do_ready (Creature* self) const
{
	// call handler
	if (!ready_cb.empty())
		ready_cb.run(self);
}

void
AI::do_heartbeat (Creature* self) const
{
	// call handler
	if (!heartbeat_cb.empty())
		heartbeat_cb.run(self);
}

void
AI::do_load (Creature* self) const
{
	// call handler
	if (!load_cb.empty())
		load_cb.run(self);
}

void
AI::do_pump (Creature* self, Scriptix::Value data) const
{
	// call handler
	if (!pump_cb.empty())
		pump_cb.run(self, data);
}

void
AI::do_save (Creature* self, ScriptRestrictedWriter* writer) const
{
	// call handler
	if (!save_cb.empty())
		save_cb.run(self, writer);
}

void
SAIManager::add (AI* new_ai)
{
	// add to list
	ai.push_back(new_ai);
}

AI*
SAIManager::get (String name)
{
	for (AIList::iterator i = ai.begin(); i != ai.end(); ++i)
		if ((*i)->name == name)
			return *i;
	return NULL;
}

int
SAIManager::initialize (void)
{
	if (require(ScriptBindings) != 0)
		return 1;
	if (require(EventManager) != 0)
		return 1;
	
	ManifestFile man(SettingsManager.get_ai_path(), S(".ai"));
	StringList files = man.get_files();;
	for (StringList::iterator i = files.begin(); i != files.end(); ++i) {
		// load ai
		AI* ai = new AI();
		if (ai == NULL) {
			fatal("new AI failed");
			return -1;
		}
		File::Reader reader;
		if (reader.open(*i))
			return -1;
		if (ai->load(reader))
			return -1;

		// make sure that the script name matches the AI id in the file
		if (base_name(*i) != ai->get_name()) {
			Log::Warning << "Ignoring AI script: path " << *i << " does not match ID " << ai->get_name();
			continue;
		}

		add(ai);
	}

	return 0;
}
void
SAIManager::shutdown (void)
{
}
