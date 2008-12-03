/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/rand.h"
#include "mud/server.h"
#include "mud/fileobj.h"
#include "mud/weather.h"
#include "mud/settings.h"
#include "mud/zone.h"
#include "mud/gametime.h"
#include "mud/clock.h"

_MWeather MWeather;

int
_MWeather::initialize ()
{
	return region.load();
}

void
_MWeather::shutdown ()
{
}

void
_MWeather::save ()
{
}

int
WeatherRegion::load (File::Reader& reader)
{
	states.clear();
	state = 0;
	ticks = 0;
	
	FO_READ_BEGIN
		FO_OBJECT("weather", "state")
			WeatherState state(node.get_name());
			FO_READ_BEGIN
				FO_ATTR("state", "id")
					state.id = node.get_string();
				FO_ATTR("state", "desc")
					state.descs.push_back(node.get_string());
				FO_OBJECT("state", "change")
					WeatherChange change;
					FO_READ_BEGIN
						FO_ATTR("change", "target")
							change.to = node.get_string();
						FO_ATTR("change", "chance")
							change.chance = node.get_int();
						FO_ATTR("change", "text")
							change.desc = node.get_string();
					FO_READ_ERROR
						throw error;
					FO_READ_END
					state.changes.push_back(change);
			FO_READ_ERROR
				throw error;
			FO_READ_END
			states.push_back(state);
		FO_ATTR("weather", "current")
			state = get_state(node.get_string());
			if (state < 0)
				throw File::Error(S("Current state out of range"));
		FO_ATTR("weather", "ticks")
			ticks = node.get_int();
			if (ticks > 500) // ludicrous
				ticks = 500;
	FO_READ_ERROR
		return -1;
	FO_READ_END

	// must have patterns
	if (states.empty()) {
		Log::Error << "Weather region has no states.";
		return -1;
	}

	// state change rules must be valid
	for (std::vector<WeatherState>::const_iterator si = states.begin(); si != states.end(); ++si)
		for (std::vector<WeatherChange>::const_iterator ci = si->changes.begin(); ci != si->changes.end(); ++ci)
			// have state?
			if (get_state(ci->to) < 0) {
				Log::Error << "Weather state " << si->id << " has a change rule to non-existant state " << ci->to << ".";
				return -1;
			}

	return 0;
}

void
WeatherRegion::save (File::Writer& writer) const
{
	for (std::vector<WeatherState>::const_iterator si = states.begin(); si != states.end(); ++si) {
		writer.begin(S("weather"), S("state"));
		writer.attr(S("state"), S("id"), si->id);
		for (StringList::const_iterator di = si->descs.begin(); di != si->descs.end(); ++di)
			writer.attr(S("state"), S("desc"), *di);
		for (std::vector<WeatherChange>::const_iterator ci = si->changes.begin(); ci != si->changes.end(); ++ci) {
			writer.begin(S("state"), S("change"));
			writer.attr(S("change"), S("target"), ci->to);
			writer.attr(S("change"), S("chance"), ci->chance);
			writer.attr(S("change"), S("text"), ci->desc);
			writer.end();
		}
		writer.end();
	}

	writer.attr(S("weather"), S("current"), states[state].id);
	writer.attr(S("weather"), S("ticks"), ticks);
}

int
WeatherRegion::load ()
{
	std::string path = MSettings.get_world_path() + "/weather";

	// open
	File::Reader reader;
	File::Node node(reader);
	if (reader.open(path)) {
		Log::Error << "Failed to open " << path << ": " << strerror(errno);
		return -1;
	}

	// load
	return load(reader);
}

int
WeatherRegion::save () const
{
	std::string path = MSettings.get_world_path() + "/weather";

	// open
	File::Writer writer;
	if (writer.open(path)) {
		Log::Error << "Failed to open " << path << ": " << strerror(errno);
		return -1;
	}

	// save
	save(writer);
	return 0;
}

int
WeatherRegion::get_state (const std::string& name) const {
	for (uint i = 0; i < states.size(); ++i)
		if (states[i].id == name)
			return i;
	return -1;
}

std::string
WeatherRegion::get_current_desc () const {
	uint i = get_random(states[state].descs.size());
	return states[state].descs[i];
}

void
WeatherRegion::update () {
	// only if we actually have any states...
	if (states.empty())
		return;

	// time for update?
	if (ticks-- == 0) {
		// random number
		uint random = get_random(100) + 1;

		// check if it fall in under any of the state changes
		// if it doesn't, then the chance just means 'don't change'
		for (std::vector<WeatherChange>::const_iterator i = states[state].changes.begin(); i != states[state].changes.end(); ++i) {
			// yes, in range
			if (random <= i->chance) {
				// get state
				int nstate = get_state(i->to);

				// valid state?
				if (nstate >= 0) {
					state = nstate;

					// announce
					MZone.announce(i->desc, ANFL_OUTDOORS);
				}
				break;
			}

			// decrement range
			random -= i->chance;
		}

		// update time
		ticks = (get_random (4) + 1) * TICKS_PER_HOUR;
	}
}
