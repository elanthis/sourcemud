/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_TIME_H
#define AWEMUD_MUD_TIME_H

#include "common/types.h"
#include "common/string.h"
#include "mud/fileobj.h"
#include "mud/server.h"
#include "common/imanager.h"
#include "mud/clock.h"
#include "common/gcvector.h"

#define TICKS_PER_HOUR ROUNDS_TO_TICKS((60 * 60) / GAME_TIME_SCALE)

#define SUN_UP_HOUR 6
#define SUN_DOWN_HOUR 19
#define SUN_CHANGE_LENGTH ROUNDS_TO_TICKS(60 * 5)

class
GameCalendar : public GC
{
	public:
	struct Month : public GC
	{
		String name;
		uint16 day_count;
		uint8 leap_years;	// each leap_years year, month has extra day

		Month () : name(), day_count(0), leap_years(0) {}
	};
	struct Holiday : public GC
	{
		String name;
		int year; // year count it occurs (every year, every three years, etc.)
		int day; // day of months it occurs
		int weekday; // weekday it occurs
		int wdindex; // count of weekday it occurs (first monday, second tuesday, etc.)
		int month; // month it occurs

		Holiday () : name(), year(0), day(0), weekday(0), wdindex(0), month(0) {}
	};
	typedef GCType::vector<Holiday> HolidayList;
	typedef GCType::vector<Month> MonthList;
	
	MonthList months;
	HolidayList holidays;
	StringList weekdays;
	StringList day_text;
	StringList night_text;
	StringList sunrise_text;
	StringList sunset_text;

	GameCalendar () : months(), holidays(), weekdays(), day_text(), night_text(), sunrise_text(), sunset_text() {}

	uint8 get_weekday (const class GameTime &) const;
	uint16 days_in_month (const class GameTime &) const;
	String get_holiday (const class GameTime &) const;

	int find_month (String name);
	int find_weekday (String name);

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
	String time_str () const;
	String date_str () const;

	String encode () const;
	int decode (String time);

	bool is_day () const { return hour >= SUN_UP_HOUR && hour < SUN_DOWN_HOUR; }
	bool is_night () const { return !is_day (); }

	void update (uint);
	void clip_time ();

	int load (File::Reader& reader);
	void save (File::Writer& writer) const;

	bool operator == (const GameTime&) const;
	bool operator != (const GameTime&) const;
};

class STimeManager : public IManager
{
	public:
	virtual int initialize ();
	virtual void shutdown ();
	virtual void save ();

	GameCalendar calendar;
	GameTime time;
};
extern STimeManager TimeManager;

#endif
