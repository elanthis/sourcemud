/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#include <algorithm>

#ifdef HAVE_REGEX
#include <regex.h>
#endif

#include <vector>

#include "common/types.h"
#include "common/string.h"
#include "common/error.h"
#include "common/log.h"

// for portability
#ifndef __va_copy
#define __va_copy(x, y) x = y
#endif

const char *yes_no[3] = {"yes","no",NULL};
const char *on_off[3] = {"on","off",NULL};
const char *true_false[3] = {"true","false",NULL};
const char *a_an[3] = {"a","an",NULL};

bool
str_is_valid_id (StringArg string)
{
	// can't be empty
	if (string.empty())
		return false;

	// must start with an alphabetic character
	if (!isalpha(string[0]))
		return false;

	// only alphabetic, numeric, and underscores allowed
	for (size_t i = 1; i < string.size(); ++i)
		if (!isalnum(string[i]) && string[i] != '_' && string[i] != '.' && string[i] != '-')
			return false;

	// passed all tests
	return true;
}

bool
str_is_number (StringArg string)
{
	// no length?  bad
	if (string.empty())
		return false;

	// iterator
	const char* c = string.c_str();

	// a starting + or - ?  good
	if (*c == '-' || *c == '+')
		c ++;

	// scan
	while (*c && isdigit (*c))
		++c;

	// stopped before end of line?  bad
	if (*c)
		return false;
		
	// finished at end of line?  good
	return true;
}

bool
str_is_email (StringArg string)
{
#ifdef HAVE_REGEX
	static RegEx regex ("^[a-z0-9._-]+@[a-z0-9_-]+(\\.[a-z0-9_-]+)+$", true);
	return regex.grep(string);

#else // HAVE_REGEX
	const char *valid =
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"-_."; // do not include @

	const char* cptr = string.c_str();

	// name part
	int len = strspn (cptr, valid);
	if (len == 0) // no valid name part
		return false;

	// after name must be @
	if (cptr[len] != '@')
		return false;

	// update cptr pointer to after @
	cptr += len + 1;

	// last part must be valid
	len = strspn (cptr, valid);
	if (len == 0)
		return false;

	// must be a . in the last part
	char *dot = strchr (cptr, '.');
	if (dot == NULL)
		return false;

	// those are good enough checks for me
	return true;

#endif // HAVE_REGEX
}

bool
str_is_true (StringArg string)
{
	return (
		str_eq(string, "true") ||
		str_eq(string, "yes") ||
		str_eq(string, "on") ||
		(str_is_number(string) && tolong(string) != 0)
	);
}

bool
str_eq (const char *str_a, const char *str_b, size_t len)
{
	// for NULL checking -- both NULL, or same, are thus equal
	if (str_a == str_b)
		return true;
	// one is NULL but other not, not same
	if (str_a == NULL || str_b == NULL)
		return false;
	// do comparison
	if (len)
		return !strncasecmp (str_a, str_b, len);
	else
		return !strcasecmp (str_a, str_b);
}

bool
phrase_match (StringArg match, StringArg test)
{
	if (!match || !test)
		return false;

	struct chunk {
		const char *base;
		int len;
	} chunks[5];
	int cchunk = 0;

	const char *c = test;
	// skip leading spaces
	while (*c != '\0' && isspace (*c))
		++ c;
	if (*c == '\0')
		return false;

	// break up test string
	cchunk = 1;
	chunks[cchunk - 1].base = c;
	while (*c != '\0') {
		// end of chunk
		if (isspace (*c)) {
			// break at max
			if (cchunk == 5)
				break;

			// close current
			chunks[cchunk - 1].len = c - chunks[cchunk - 1].base;

			// find next
			while (*c != '\0' && isspace (*c))
				++ c;
			if (*c == '\0')
				break;
			++ cchunk;
			chunks[cchunk - 1].base = c;
		} else
			++ c;
	}
	// close last on on stack
	chunks[cchunk - 1].len = c - chunks[cchunk - 1].base;

	// compare chunks to match string
	int matches = 0;
	c = match;
	const char *word = c;
	while (*c != '\0') {
		// end of chunk
		if (isspace (*c)) {
			// compare current chunks to this word
			if (chunks[matches].len <= c - word && !strncasecmp (word, chunks[matches].base, chunks[matches].len)) {
				// match
				++ matches;
				if (matches == cchunk)
					return true;
			}

			// find next word
			while (*c != '\0' && isspace (*c))
				++ c;
			if (*c == '\0')
				break;
			word = c;
		} else
			++ c;
	}
	if (chunks[matches].len <= c - word && !strncasecmp (word, chunks[matches].base, chunks[matches].len)) {
		// match
		++ matches;
	}

	// matched all chunks?
	return matches == cchunk ? true : false;
}

const char *
get_num_suffix (unsigned int num) {
	if (num == 11 || num == 12 || num == 13) { return "th"; }
	num %= 10;
	return num == 1 ? "st" : num == 2 ? "nd" : num == 3 ? "rd" : "th";
}

int
get_index_of (const char **list, StringArg str, int def)
{
	assert (list != NULL);

	if (str.empty())
		return def;

	for (int i = 0; list[i] != NULL; i ++)
		if (str_eq (list[i], str))
			return i;
	return def;
}

bool
get_true_false (StringArg string)
{
	if (!get_index_of (on_off, string.c_str()))
		return true;
	if (!get_index_of (yes_no, string.c_str()))
		return true;
	if (!get_index_of (true_false, string.c_str()))
		return true;

	return false;
}

uint
str_value (StringArg string)
{
	// empty?  none
	if (string.empty())
		return 0;

	// first, check for #<number> notation or .<number> notation
	if (string[0] == '#' || string[0] == '.') {
		// empty, not valid
		if (string[1] == '\0')
			return 0;

		char *end;
		uint ret = strtoul (string.c_str() + 1, &end, 10);

		// not a full valid number
		if (end == NULL || *end != '\0')
			return 0;

		return ret;
	}

	// next, look for "other"
	if (str_eq (string, "other")) {
		return 2; // second item
	}

	// ok, scan for first, second, third, fourth... ninth
	static const char *numerics[] = { "first", "second", "third",
		"fourth", "fifth", "sixth", "seventh", "eighth",
		"ninth", NULL};
	int ret = get_index_of (numerics, string);
	if (ret >= 0) {
		return ret + 1; /* first is position 0,
				   but is item 1 - add 1 */
	}

	// try the number with th, st, nd, and rd on the end
	if (string.size() >= 3) {
		// parse
		char* end;
		int value = strtol(string.c_str(), &end, 10);

		// invalid?  end
		if (value <= 0)
			return 0;

		// check that rest of string is 1st, 2nd, 3rd, or Xth
		if (str_eq(end - 1, "1st"))
			return value;
		else if (str_eq(end - 1, "2nd"))
			return value;
		else if (str_eq(end - 1, "3rd"))
			return value;
		else if (str_eq(end, "th"))
			return value;
	}

	// no matches - no a number
	return 0;
}

StringList&
explode (StringList& list, StringArg str, char ch)
{
	size_t l, i;

	list.clear();

	if (str.empty())
		return list;

	// break up str by the ch
	l = 0;
	while ((i = str.find(ch, l)) != String::npos) {
		list.push_back(str.substr(l, i-l));
		l = i + 1;
	}
	list.push_back(str.substr(l));

	return list;
}

String&
implode (String& string, const StringList& list, char ch)
{
	string.clear();

	// keep adding to string
	for (StringList::const_iterator i = list.begin(); i != list.end(); ++i) {
		if (!string.empty())
			string += ch;
		string += *i;
	}

	return string;
}

String&
capwords (String& out, StringArg string)
{
	bool space = true;
	char ch;
	BaseString temp;
	temp.resize(string.size());
	for (size_t i = 0; i < string.size(); ++i) {
		ch = string[i];
		if (isspace(ch)) {
			space = true;
		} else if (space) {
			ch = toupper(ch);
			space = false;
		}
		temp[i] = ch;
	}
	out = temp;
	return out;
}

String
tostr (long num)
{
	StringBuffer str;
	str << num;
	return str.str();
}

long
tolong (StringArg str)
{
	return strtol(str.c_str(), NULL, 10);
}

// ----- String class stuff -----

String
strip (StringArg string)
{
	if (string.empty())
		return String();

	// strip back
	size_t back = string.find_last_not_of(" \t\n");

	// strip front
	size_t front = string.find_first_not_of(" \t\n");

	// do substr
	return string.substr(front, back - front + 1);
}

String
strupper (StringArg string)
{
	String ret = string;
	std::transform(ret.begin(), ret.end(), ret.begin(), toupper);
	return ret;
}

String
strlower (StringArg string)
{
	String ret = string;
	std::transform(ret.begin(), ret.end(), ret.begin(), tolower);
	return ret;
}

namespace {
	struct Replace {
		Replace (StringArg s_from, StringArg s_to) : from(s_from), to (s_to) {}

		const String& from;
		const String& to;

		char operator ()(char c) {
			char* r = strchr(from.c_str(), c);
			if (r != NULL)
				return to[(int)(from.c_str() - r)];
			return c;
		}
	};
}

String
str_tr (StringArg source, StringArg from, StringArg to)
{
	assert(from.size() == to.size());

	String ret = source;
	std::transform(ret.begin(), ret.end(), ret.begin(), Replace(from,to));
	return ret;
}

String
trim (StringArg source, StringArg accept)
{
	StringBuffer ret;

	for (String::const_iterator i = source.begin(); i != source.end(); ++i)
		if (strchr(accept.c_str(), *i))
			ret << *i;

	return ret.str();
}
