/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_TIME_H
#define SOURCEMUD_MUD_TIME_H

#include "common.h"
#include "common/types.h"
#include "common/imanager.h"
#include "mud/fileobj.h"
#include "mud/server.h"
#include "mud/clock.h"

#define TICKS_PER_HOUR ROUNDS_TO_TICKS((60 * 60) / GAME_TIME_SCALE)

#define SUN_UP_HOUR 6
#define SUN_DOWN_HOUR 19
#define SUN_CHANGE_LENGTH ROUNDS_TO_TICKS(60 * 5)

class
GameCalendar
{
	public:
	struct Month
	{
		std::string name;
		uint16 day_count;
		uint8 leap_years;	// each leap_years year, month has extra day

		Month () : name(), day_count(0), leap_years(0) {}
	};
	struct Holiday
	{
		std::string name;
		int year; // year count it occurs (every year, every three years, etc.)
		int day; // day of months it occurs
		int weekday; // weekday it occurs
		int wdindex; // count of weekday it occurs (first monday, second tuesday, etc.)
		int month; // month it occurs

		Holiday () : name(), year(0), day(0), weekday(0), wdindex(0), month(0) {}
	};
	typedef std::vector<Holiday> HolidayList;
	typedef std::vector<Month> MonthList;
	
	MonthList months;
	HolidayList holidays;
	std::vector<std::string> weekdays;
	std::vector<std::string> day_text;
	std::vector<std::string> night_text;
	std::vector<std::string> sunrise_text;
	std::vector<std::string> sunset_text;

	GameCalendar () : months(), holidays(), weekdays(), day_text(), night_text(), sunrise_text(), sunset_text() {}

	uint8 get_weekday (const class GameTime &) const;
	uint16 days_in_month (const class GameTime &) const;
	std::string get_holiday (const class GameTime &) const;

	int find_month (const std::string& name);
	int find_weekday (const std::string& name);

	int load ();
};

class
GameTime
{
	private:
	uint16 year;
	uint16 day;
	uint16 ticks_in_hour;
	uint8 month;
	uint8 hour;

	public:
	GameTime () : year(0), day(1), ticks_in_hour(0), month(1), hour(0) {}

	uint get_year() const { return year; }
	uint set_year(uint val) { year = val; clip_time(); return year; }
	uint16 get_month() const { return month; }
	uint16 set_month(uint16 val) { month = val; clip_time(); return month; }
	uint16 get_day() const { return day; }
	uint16 set_day(uint16 val) { day = val; clip_time(); return day; }
	uint8 get_hour() const { return hour; }
	uint8 set_hour(uint8 val) { hour = val; clip_time(); return hour; }
	uint16 get_ticks() const { return ticks_in_hour; }
	uint16 set_ticks(uint16 val) { ticks_in_hour = val; clip_time(); return ticks_in_hour; }
	uint8 get_minutes() const { return ticks_in_hour * 60 / TICKS_PER_HOUR; }

	void time_str (char *buf, int len) const;
	void date_str (char *buf, int len) const;
	std::string time_str () const;
	std::string date_str () const;

	std::string encode () const;
	int decode (const std::string& time);

	bool is_day () const { return hour >= SUN_UP_HOUR && hour < SUN_DOWN_HOUR; }
	bool is_night () const { return !is_day (); }

	void update (uint);
	void clip_time ();

	int load (File::Reader& reader);
	void save (File::Writer& writer) const;

	bool operator == (const GameTime&) const;
	bool operator != (const GameTime&) const;
};

class _MTime : public IManager
{
	public:
	virtual int initialize ();
	virtual void shutdown ();
	virtual void save ();

	GameCalendar calendar;
	GameTime time;
};
extern _MTime MTime;

#endif
