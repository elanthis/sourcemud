/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_TIME_H
#define SOURCEMUD_MUD_TIME_H

#include "common/types.h"
#include "common/imanager.h"
#include "mud/fileobj.h"
#include "mud/server.h"
#include "mud/clock.h"

#define TICKS_PER_HOUR ROUNDS_TO_TICKS((60 * 60) / GAME_TIME_SCALE)

#define SUN_UP_HOUR 6
#define SUN_DOWN_HOUR 19
#define SUN_CHANGE_LENGTH ROUNDS_TO_TICKS(60 * 5)

class GameCalendar
{
public:
	struct Month {
		std::string name;
		uint16 day_count;
		uint8 leap_years;	// each leap_years year, month has extra day

		Month() : name(), day_count(0), leap_years(0) {}
	};
	struct Holiday {
		std::string name;
		int year; // year count it occurs (every year, every three years, etc.)
		int day; // day of months it occurs
		int weekday; // weekday it occurs
		int wdindex; // count of weekday it occurs (first monday, second tuesday, etc.)
		int month; // month it occurs

		Holiday() : name(), year(0), day(0), weekday(0), wdindex(0), month(0) {}
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

	GameCalendar() : months(), holidays(), weekdays(), day_text(), night_text(), sunrise_text(), sunset_text() {}

	uint8 getWeekday(const class GameTime &) const;
	uint16 daysInMonth(const class GameTime &) const;
	std::string getHoliday(const class GameTime &) const;

	int findMonth(const std::string& name);
	int findWeekday(const std::string& name);

	int load();
};

class GameTime
{
private:
	uint16 year;
	uint16 day;
	uint16 ticks_in_hour;
	uint8 month;
	uint8 hour;

public:
	GameTime() : year(0), day(1), ticks_in_hour(0), month(1), hour(0) {}

	uint getYear() const { return year; }
	uint setYear(uint val) { year = val; clipTime(); return year; }
	uint16 getMonth() const { return month; }
	uint16 setMonth(uint16 val) { month = val; clipTime(); return month; }
	uint16 getDay() const { return day; }
	uint16 setDay(uint16 val) { day = val; clipTime(); return day; }
	uint8 getHour() const { return hour; }
	uint8 setHour(uint8 val) { hour = val; clipTime(); return hour; }
	uint16 getTicks() const { return ticks_in_hour; }
	uint16 setTicks(uint16 val) { ticks_in_hour = val; clipTime(); return ticks_in_hour; }
	uint8 getMinutes() const { return ticks_in_hour * 60 / TICKS_PER_HOUR; }

	void timeStr(char *buf, int len) const;
	void dateStr(char *buf, int len) const;
	std::string timeStr() const;
	std::string dateStr() const;

	std::string encode() const;
	int decode(const std::string& time);

	bool isDay() const { return hour >= SUN_UP_HOUR && hour < SUN_DOWN_HOUR; }
	bool isNight() const { return !isDay(); }

	void update(uint);
	void clipTime();

	int load(File::Reader& reader);
	void save(File::Writer& writer) const;

	bool operator == (const GameTime&) const;
	bool operator != (const GameTime&) const;
};

class _MTime : public IManager
{
public:
	virtual int initialize();
	virtual void shutdown();
	virtual void save();

	GameCalendar calendar;
	GameTime time;
};
extern _MTime MTime;

#endif
