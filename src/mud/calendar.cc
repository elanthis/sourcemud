/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/log.h"
#include "common/string.h"
#include "mud/gametime.h"
#include "mud/settings.h"
#include "mud/filetab.h"

int GameCalendar::load()
{
	std::string path = MSettings.getMiscPath() + "/calendar";

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
		std::string type = reader.get(i, 0);

		// day time description
		if (type == "daytime") {
			std::string time = reader.get(i, 1);
			std::string data = reader.get(i, 2);
			if (data.empty()) {
				Log::Error << "Calendar error: " << path << ',' << reader.getLine(i) << ": Missing data for daytime entry";
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
				Log::Error << "Calendar error: " << path << ',' << reader.getLine(i) << ": Unknown time '" << time << "' for daytime entry";
				return -1;
			}

			// weekday
		} else if (type == "weekday") {
			std::string name = reader.get(i, 1);
			if (name.empty()) {
				Log::Error << "Calendar error: " << path << ',' << reader.getLine(i) << ": Missing name for weekday entry";
				return -1;
			}

			weekdays.push_back(name);

			// month
		} else if (type == "month") {
			std::string name = reader.get(i, 1);
			ulong days = tolong(reader.get(i, 2));
			ulong leap = tolong(reader.get(i, 3));

			if (name.empty()) {
				Log::Error << "Calendar error: " << path << ',' << reader.getLine(i) << ": Missing name for month entry";
				return -1;
			}

			GameCalendar::Month month;
			month.name = name;
			month.day_count = days;
			month.leap_years = leap;
			months.push_back(month);

			// unknown
		} else {
			Log::Error << "Calendar error: " << path << ',' << reader.getLine(i) << ": Unknown entry type";
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

uint8 GameCalendar::getWeekday(const GameTime &gt) const
{
	uint64 total_days = 0;

	// what the hell was this supposed to be doing again?

	for (std::vector<GameCalendar::Month>::const_iterator i = months.begin(); i != months.end(); i ++) {
		uint year = gt.getYear() - (gt.getMonth() > months.end() - i + 1 ? 0 : 1);
		total_days += i->day_count * year;
		if (i->leap_years && year % i->leap_years == 0)
			total_days ++;
	}
	total_days += gt.getDay();

	if (weekdays.size())
		return total_days % weekdays.size();
	else
		return 0;
}

uint16 GameCalendar::daysInMonth(const GameTime &gt) const
{
	if (gt.getMonth() < 1 || gt.getMonth() > months.size())
		return 0;
	if (months[gt.getMonth() - 1].leap_years && (gt.getYear() % months[gt.getMonth() - 1].leap_years) == 0)
		return months[gt.getMonth() - 1].day_count + 1;
	else
		return months[gt.getMonth() - 1].day_count;
}

std::string GameCalendar::getHoliday(const GameTime &gt) const
{
	for (std::vector<GameCalendar::Holiday>::const_iterator i = holidays.begin(); i != holidays.end(); ++i) {
		// check year
		if (i->year > 0 && (gt.getYear() % i->year) != 0)
			continue;
		// check month
		if (i->month > 0 && gt.getMonth() != i->month)
			continue;
		// check day
		if (i->day > 0 && gt.getDay() != i->day)
			continue;
		// a match - weee
		return i->name;
	}

	return std::string();
}

int GameCalendar::findMonth(const std::string& name)
{
	uint ii = 0;
	for (std::vector<GameCalendar::Month>::const_iterator i = months.begin(); i != months.end(); ++i, ++ii) {
		if (name == i->name)
			return ii;
	}
	return -1;
}

int GameCalendar::findWeekday(const std::string& name)
{
	uint ii = 0;
	for (std::vector<std::string>::const_iterator i = weekdays.begin(); i != weekdays.end(); ++i, ++ii) {
		if (name == *i)
			return ii;
	}
	return -1;
}
