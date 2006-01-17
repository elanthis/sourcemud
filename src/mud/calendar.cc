/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "gametime.h"
#include "fileobj.h"
#include "settings.h"
#include "log.h"

int
GameCalendar::load (void)
{
	String path = SettingsManager.get_misc_path() + "/calendar";

	// open
	File::Reader reader;
	if (reader.open(path)) {
		Log::Error << "Failed to open " << path << ": " << strerror(errno);
		return -1;
	}

	// do read
	FO_READ_BEGIN
		// a month
		FO_OBJECT("month")
			GameCalendar::Month month;

			// read month attribute
			FO_READ_BEGIN
				FO_ATTR("name")
					month.name = node.get_data();
				// days in month
				FO_ATTR("days")
					FO_TYPE_ASSERT(INT);
					month.day_count = tolong(node.get_data());
				// leap year count
				FO_ATTR("leap")
					FO_TYPE_ASSERT(INT);
					month.leap_years = tolong(node.get_data());
			FO_READ_ERROR
				return -1;
			FO_READ_END

			// must have a name
			if (!month.name) {
				Log::Error << "Month has no name at " << reader.get_filename() << ':' << node.get_line();
				return -1;
			}

			// add month
			months.push_back (month);

		// holiday?
		FO_OBJECT("holiday")
			GameCalendar::Holiday holiday;

			// read holiday attrs
			FO_READ_BEGIN
				FO_ATTR("name")
					holiday.name = node.get_name();
				// day of month
				FO_ATTR("day")
					FO_TYPE_ASSERT(INT)
					holiday.day = tolong(node.get_data());
				// month?
				FO_ATTR("month")
					holiday.month = find_month(node.get_data());
					if (holiday.month < 0) {
						holiday.month = 0;
						Log::Error << "Unknown month '" << node.get_data() << "' at " << reader.get_filename() << ':' << node.get_line();
						return -1;
					}
				// years repeat?
				FO_ATTR("years")
					FO_TYPE_ASSERT(INT)
					holiday.year = tolong(node.get_data());
				// day of week?
				FO_ATTR("weekday")
					holiday.weekday = find_weekday(node.get_data());
					if (holiday.weekday < 0) {
						holiday.weekday = 0;
						Log::Error << "Unknown weekday '" << node.get_data() << "' at " << reader.get_filename() << ':' << node.get_line();
						return -1;
					}
				// weekday index?  (like 2nd tuesday)
				FO_ATTR("wdindex")
					FO_TYPE_ASSERT(INT)
					holiday.wdindex = tolong(node.get_data());
			FO_READ_ERROR
				return -1;
			FO_READ_END

			// must have a name
			if (!holiday.name) {
				Log::Error << "Holiday has no name at " << reader.get_filename() << ':' << node.get_line();
				return -1;
			}

			// add holiday
			holidays.push_back (holiday);
		
		// weekday?
		FO_ATTR("weekday")
			weekdays.push_back(node.get_data());
		// daytyime desc text?
		FO_ATTR("day")
			day_text.push_back(node.get_data());
		// nighttime desc text?
		FO_ATTR("night")
			night_text.push_back(node.get_data());
		// sun-rising text?
		FO_ATTR("sunrise")
			sunrise_text.push_back(node.get_data());
		// sun-setting text?
		FO_ATTR("sunset")
			sunset_text.push_back(node.get_data());
	FO_READ_ERROR
		return -1;
	FO_READ_END

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
GameCalendar::find_month (StringArg name)
{
	uint ii = 0;
	for (GCType::vector<GameCalendar::Month>::const_iterator i = months.begin(); i != months.end(); ++i, ++ii) {
		if (name == i->name)
			return ii;
	}
	return -1;
}

int
GameCalendar::find_weekday (StringArg name)
{
	uint ii = 0;
	for (StringList::const_iterator i = weekdays.begin(); i != weekdays.end(); ++i, ++ii) {
		if (name == *i)
			return ii;
	}
	return -1;
}
