/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_WEATHER_H
#define SOURCEMUD_MUD_WEATHER_H

#include <vector>
#include "mud/fileobj.h"
#include "common/string.h"
#include "common/imanager.h"

// description of a change in the weather, such as 'the rain starts falling more heavily'
struct WeatherChange {
	std::string to;	// target state
	std::string desc;	// message when changing to state
	uint chance;	// chance of occuring
};

// a weather state, such as 'raining heavily' or 'snowing lightly'
struct WeatherState {
	inline WeatherState (std::string s_id) : id(s_id), changes(), descs() {}
		
	int load (File::Reader& reader);
	void save (File::Writer& writer) const;

	std::string id;
	std::vector<WeatherChange> changes;
	StringList descs;
};

// a single region of the world and its weather patterns
class WeatherRegion {
	public:
	WeatherRegion () : states(), state(0), ticks(0) {}

	int load (File::Reader& reader);
	void save (File::Writer& writer) const;

	int load ();
	int save () const;

	std::string get_current_desc () const;

	void update ();

	protected:
	std::vector<WeatherState> states;
	uint state;
	uint ticks;

	int get_state (std::string name) const;
};

// manage all regions
// FIXME: we should actually *have* multiple regions... ^^;
class _MWeather : public IManager
{
	public:
	virtual int initialize ();
	virtual void shutdown ();
	virtual void save ();

	// update the manager (once per tick)
	inline void update () { region.update(); }

	// get current weather description
	inline std::string get_current_desc () const { return region.get_current_desc(); }

	private:
	WeatherRegion region;
};
extern _MWeather MWeather;

#endif
