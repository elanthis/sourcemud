/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <dirent.h>
#include <fnmatch.h>

#include "mud/settings.h"
#include "mud/char.h"
#include "mud/ai.h"
#include "mud/room.h"
#include "mud/eventids.h"
#include "mud/bindings.h"

SAIManager AIManager;

AI::AI (StringArg s_name) : name(s_name) {} 

int
AI::load (File::Reader& reader)
{
	FO_READ_BEGIN
		FO_ATTR("load")
			load_cb = Scriptix::ScriptFunction::compile(S("load"), node.get_data(), S("self"), reader.get_filename(), node.get_line());
			
			if (load_cb.empty())
				throw File::Error(S("Script compile failed"));
		FO_ATTR("save")
			save_cb = Scriptix::ScriptFunction::compile(S("save"), node.get_data(), S("self,writer"), reader.get_filename(), node.get_line());
			
			if (save_cb.empty())
				throw File::Error(S("Script compile failed"));
		FO_ATTR("heartbeat")
			heartbeat_cb = Scriptix::ScriptFunction::compile(S("heartbeat"), node.get_data(), S("self"), reader.get_filename(), node.get_line());
			
			if (heartbeat_cb.empty())
				throw File::Error(S("Script compile failed"));
		FO_ATTR("ready")
			ready_cb = Scriptix::ScriptFunction::compile(S("ready"), node.get_data(), S("self"), reader.get_filename(), node.get_line());
			
			if (ready_cb.empty())
				throw File::Error(S("Script compile failed"));
		FO_ATTR("pump")
			pump_cb = Scriptix::ScriptFunction::compile(S("pump"), node.get_data(), S("self,data"), reader.get_filename(), node.get_line());
			
			if (pump_cb.empty())
				throw File::Error(S("Script compile failed"));
		FO_KEYED("event")
			EventID event = EventID::lookup(node.get_key());
			if (event.valid()) {
				Scriptix::ScriptFunction handler = EventManager.compile(event, node.get_data(), reader.get_filename(), node.get_line());

				if (handler.empty())
					throw File::Error(S("Script compile failed"));

				event_cb[EventID::lookup(node.get_name())] = handler;
			}
	FO_READ_ERROR
		return -1;
	FO_READ_END

	return 0;
}

void
AI::do_event (Character* self, const Event& event) const
{
	// get handler
	EventList::const_iterator i = event_cb.find(event.get_id());
	if (i != event_cb.end())
		i->second.run(self, EventID::nameof(event.get_id()), event.get_actor(), event.get_room(), event.get_data(0), event.get_data(1), event.get_data(2), event.get_data(3), event.get_data(4));
}

void
AI::do_ready (Character* self) const
{
	// call handler
	if (!ready_cb.empty())
		ready_cb.run(self);
}

void
AI::do_heartbeat (Character* self) const
{
	// call handler
	if (!heartbeat_cb.empty())
		heartbeat_cb.run(self);
}

void
AI::do_load (Character* self) const
{
	// call handler
	if (!load_cb.empty())
		load_cb.run(self);
}

void
AI::do_pump (Character* self, Scriptix::Value data) const
{
	// call handler
	if (!pump_cb.empty())
		pump_cb.run(self, data);
}

void
AI::do_save (Character* self, ScriptRestrictedWriter* writer) const
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
SAIManager::get (StringArg name)
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
	
	DIR *dir;
	dirent *dent;

	Log::Info << "Loading AI packs";

	// read directory
	dir = opendir (SettingsManager.get_ai_path());
	if (dir != NULL) {
		while ((dent = readdir (dir)) != NULL) {
			if (!fnmatch ("*.ai", dent->d_name, 0)) {
				StringBuffer file;
				file << SettingsManager.get_ai_path() << '/' << dent->d_name;
				String name(dent->d_name, strlen(dent->d_name) - 3);

				// load ai
				AI* ai = new AI(name);
				if (ai == NULL) {
					fatal("new AI failed");
					closedir(dir);
					return -1;
				}
				File::Reader reader;
				if (reader.open(file.str())) {
					Log::Error << "Failed to open " << file.str();
					return -1;
				}
				if (ai->load(reader))
					return -1;

				add(ai);
			}
		}
	}
	closedir (dir);

	return 0;
}
void
SAIManager::shutdown (void)
{
}
