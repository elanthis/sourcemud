/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "mud/gametime.h"
#include "mud/settings.h"
#include "mud/server.h"
#include "mud/clock.h"

STimeManager TimeManager;

void
GameTime::time_str (char *buf, int len) const
{
	snprintf (buf, len, "%d:%02d %s",
			hour == 0 ? 12 : (hour <= 12 ? hour : hour - 12),
			ticks_in_hour * 60 / TICKS_PER_HOUR,
			(hour < 12) ? "am" : "pm");
}

std::string
GameTime::time_str () const
{
	char buffer[32];
	time_str(buffer, sizeof(buffer));
	return std::string(buffer);
}

void
GameTime::date_str (char *buf, int len) const
{
	// base date string
	size_t add = snprintf (buf, len, "%s, %d%s of %s, %d",
			TimeManager.calendar.weekdays[TimeManager.calendar.get_weekday (*this)].c_str(),
			day,
			get_num_suffix (day).c_str(),
			TimeManager.calendar.months[month - 1].name.c_str(),
			year);
	// append holiday if we have one
	std::string holiday = TimeManager.calendar.get_holiday(*this);
	if (!holiday.empty()) {
		snprintf (buf + add, len - add, " (%s)", holiday.c_str());
	}
}

std::string
GameTime::date_str () const
{
	char buffer[256];
	date_str(buffer, sizeof(buffer));
	return std::string(buffer);
}

void
GameTime::update (uint ticks)
{
	ticks_in_hour += ticks;
	clip_time ();
}

void
GameTime::clip_time ()
{
	while (ticks_in_hour >= TICKS_PER_HOUR)
	{
		ticks_in_hour -= TICKS_PER_HOUR;
		hour ++;
	}
	while (hour >= 24)
	{
		hour -= 24;
		day ++;
	}
	while (day > TimeManager.calendar.days_in_month (*this))
	{
		day -= TimeManager.calendar.days_in_month (*this);
		month ++;
	}
	if (day < 1)
		day = 1;
	while (month > TimeManager.calendar.months.size ())
	{
		month -= TimeManager.calendar.months.size ();
		year ++;
	}
	if (month < 1)
		month = 1;
}

std::string
GameTime::encode () const
{
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%04d/%02d/%02d %02d.%02d", year, month, day, hour, ticks_in_hour);
	return std::string(buffer);
}

int
GameTime::decode (std::string str)
{
	uint s_year, s_month, s_day, s_hour, s_ticks;
	if (sscanf(str.c_str(), "%u/%u/%u %u.%u", &s_year, &s_month, &s_day, &s_hour, &s_ticks) != 5)
		return -1;
	year = s_year;
	month = s_month;
	day = s_day;
	hour = s_hour;
	ticks_in_hour = s_ticks;
	return 0;
}

bool
GameTime:: operator == (const GameTime& other) const
{
	return year == other.year &&
		month == other.month &&
		day == other.day &&
		hour == other.hour &&
		ticks_in_hour == other.ticks_in_hour;
}
bool
GameTime:: operator != (const GameTime& other) const
{
	return year != other.year ||
		month != other.month ||
		day != other.day ||
		hour != other.hour ||
		ticks_in_hour != other.ticks_in_hour;
}

int
STimeManager::initialize ()
{
	// initialize calendar
	if (calendar.load())
		return 1;

	File::Reader reader;

	if (reader.open(SettingsManager.get_world_path() + "/" + "time"))
		return 1;

	FO_READ_BEGIN
		FO_ATTR("time", "current")
			time.decode(node.get_string());
	FO_READ_ERROR
		return -1;
	FO_READ_END

	return 0;
}

void
STimeManager::save ()
{
	// open
	File::Writer writer;
	
	if (writer.open(SettingsManager.get_world_path() + "/" + "time"))
		return;

	// save
	writer.attr(S("time"), S("current"), time.encode());

	writer.close();
}

void
STimeManager::shutdown ()
{
}
