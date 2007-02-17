/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/gametime.h"
#include "mud/settings.h"
#include "common/log.h"
#include "mud/filetab.h"

int
GameCalendar::load (void)
{
	String path = SettingsManager.get_misc_path() + "/calendar";

	// open
	File::TabReader reader;
	if (reader.open(path)) {
		Log::Error << "Failed to open " << path << ": " << strerror(errno);
		return -1;
	}
	if (reader.load()) {
		Log::Error << "Failed to open " << path << ": " << strerror(errno);
		return -1;
	}

	// read each entry
	for (size_t i = 0; i < reader.size(); ++i) {
		String type = reader.get(i, 0);

		// day time description
		if (type == "daytime") {
			String time = reader.get(i, 1);
			String data = reader.get(i, 2);
			if (data.empty()) {
				Log::Error << "Macro error: " << path << ',' << reader.get_line(i) << ": Missing data for daytime entry";
				return -1;
			}

			if (time == "day")
				day_text.push_back(reader.get(i, 1));
			else if (time == "night")
				night_text.push_back(reader.get(i, 1));
			else if (time == "sunrise")
				sunrise_text.push_back(reader.get(i, 1));
			else if (time == "sunset")
				sunset_text.push_back(reader.get(i, 1));
			else {
				Log::Error << "Macro error: " << path << ',' << reader.get_line(i) << ": Unknown time '" << time << "' for daytime entry";
				return -1;
			}

		// weekday
		} else if (type == "weekday") {
			String name = reader.get(i, 1);
			if (name.empty()) {
				Log::Error << "Macro error: " << path << ',' << reader.get_line(i) << ": Missing name for weekday entry";
				return -1;
			}

			weekdays.push_back(name);

		// month
		} else if (type == "month") {
			String name = reader.get(i, 1);
			ulong days = tolong(reader.get(i, 2));
			ulong leap = tolong(reader.get(i, 3));

			if (name.empty()) {
				Log::Error << "Macro error: " << path << ',' << reader.get_line(i) << ": Missing name for month entry";
				return -1;
			}

			GameCalendar::Month month;
			month.name = name;
			month.day_count = days;
			month.leap_years = leap;
			months.push_back(month);

		// unknown
		} else {
			Log::Error << "Macro error: " << path << ',' << reader.get_line(i) << ": Unknown entry type";
			return -1;
		}
	}

	// CHECK MONTHS
	if (!months.size()) {
		Log::Error << "No months defined in calendar";
		return -1;
	}

	// CHECK WEEKDAYS
	if (!weekdays.size()) {
		Log::Error << "No weekdays defined in calendar";
		return -1;
	}

	return 0;
}

uint8
GameCalendar::get_weekday (const GameTime &gt) const
{
	uint64 total_days = 0;

	// what the hell was this supposed to be doing again?

	for (GCType::vector<GameCalendar::Month>::const_iterator i = months.begin (); i != months.end (); i ++)
	{
		uint year = gt.get_year() - (gt.get_month() > months.end () - i + 1 ? 0 : 1);
		total_days += i->day_count * year;
		if (i->leap_years && year % i->leap_years == 0)
			total_days ++;
	}
	total_days += gt.get_day();

	if (weekdays.size())
		return total_days % weekdays.size ();
	else
		return 0;
}

uint16
GameCalendar::days_in_month (const GameTime &gt) const
{
	if (gt.get_month() < 1 || gt.get_month() > months.size ())
		return 0;
	if (months[gt.get_month() - 1].leap_years && (gt.get_year() % months[gt.get_month() - 1].leap_years) == 0)
		return months[gt.get_month() - 1].day_count + 1;
	else
		return months[gt.get_month() - 1].day_count;
}

String
GameCalendar::get_holiday (const GameTime &gt) const
{
	for (GCType::vector<GameCalendar::Holiday>::const_iterator i = holidays.begin(); i != holidays.end(); ++i) {
		// check year
		if (i->year > 0 && (gt.get_year() % i->year) != 0)
			continue;
		// check month
		if (i->month > 0 && gt.get_month() != i->month)
			continue;
		// check day
		if (i->day > 0 && gt.get_day() != i->day)
			continue;
		// a match - weee
		return i->name;
	}

	return String();
}

int
GameCalendar::find_month (String name)
{
	uint ii = 0;
	for (GCType::vector<GameCalendar::Month>::const_iterator i = months.begin(); i != months.end(); ++i, ++ii) {
		if (name == i->name)
			return ii;
	}
	return -1;
}

int
GameCalendar::find_weekday (String name)
{
	uint ii = 0;
	for (StringList::const_iterator i = weekdays.begin(); i != weekdays.end(); ++i, ++ii) {
		if (name == *i)
			return ii;
	}
	return -1;
}
