/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_WEATHER_H
#define SOURCEMUD_MUD_WEATHER_H

#include "common/gcvector.h"
#include "mud/fileobj.h"
#include "common/string.h"
#include "common/imanager.h"

// description of a change in the weather, such as 'the rain starts falling more heavily'
struct WeatherChange {
	String to;	// target state
	String desc;	// message when changing to state
	uint chance;	// chance of occuring
};

// a weather state, such as 'raining heavily' or 'snowing lightly'
struct WeatherState {
	inline WeatherState (String s_id) : id(s_id), changes(), descs() {}
		
	int load (File::Reader& reader);
	void save (File::Writer& writer) const;

	String id;
	GCType::vector<WeatherChange> changes;
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

	String get_current_desc () const;

	void update ();

	protected:
	GCType::vector<WeatherState> states;
	uint state;
	uint ticks;

	int get_state (String name) const;
};

// manage all regions
// FIXME: we should actually *have* multiple regions... ^^;
class SWeatherManager : public IManager
{
	public:
	virtual int initialize ();
	virtual void shutdown ();
	virtual void save ();

	// update the manager (once per tick)
	inline void update () { region.update(); }

	// get current weather description
	inline String get_current_desc () const { return region.get_current_desc(); }

	private:
	WeatherRegion region;
};
extern SWeatherManager WeatherManager;

#endif
