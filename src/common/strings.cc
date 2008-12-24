/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/types.h"
#include "common/string.h"
#include "common/error.h"
#include "common/log.h"

// for portability
#ifndef __va_copy
#define __va_copy(x, y) x = y
#endif

bool str_is_valid_id(const std::string& string)
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

bool str_is_number(const std::string& string)
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

bool str_is_email(const std::string& string)
{
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
}

bool str_is_true(const std::string& string)
{
	return (
		str_eq(string, "true") ||
		str_eq(string, "yes") ||
		str_eq(string, "on") ||
		(str_is_number(string) && tolong(string) != 0)
	);
}

bool str_is_false(const std::string& string)
{
	return (
		str_eq(string, "false") ||
		str_eq(string, "no") ||
		str_eq(string, "off") ||
		string == "0"
	);
}

bool str_eq(const std::string& str_a, const std::string& str_b, size_t len)
{
	// do comparison
	if (len)
		return !strncasecmp(str_a.c_str(), str_b.c_str(), len);
	else
		return !strcasecmp(str_a.c_str(), str_b.c_str());
}

bool
prefix_match (const char* string, const char* prefix)
{
	assert(string != NULL);
	assert(prefix != NULL);

	return strncasecmp(string, prefix, strlen(prefix)) == 0;
}

bool phrase_match (const char* match, const char* test)
{
	assert(match != NULL);
	assert(test != NULL);

	if (match[0] == 0 || test[0] == 0)
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

std::string
get_num_suffix (unsigned int num) {
	if (num == 11 || num == 12 || num == 13) { return "th"; }
	num %= 10;
	return num == 1 ? "st" : num == 2 ? "nd" : num == 3 ? "rd" : "th";
}

std::vector<std::string>&
explode (std::vector<std::string>& list, const std::string& str, char ch)
{
	list.clear();

	if (str.empty())
		return list;

	// break up str by the ch
	const char* l;
	const char* i;
	l = str.c_str();
	while ((i = strchr(l, ch)) != NULL) {
		list.push_back(std::string(l, i - l));;
		l = i + 1;
	}
	list.push_back(std::string(l));

	return list;
}

std::string& implode(std::string& string, const std::vector<std::string>& list, char ch)
{
	StringBuffer buffer;

	// keep adding to string
	for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i) {
		if (!buffer.empty())
			buffer << ch;
		buffer << *i;
	}

	return string = buffer.str();
}

std::string& capwords(std::string& out, const std::string& string)
{
	bool space = true;
	char ch;
	StringBuffer buffer;
	for (size_t i = 0; i < string.size(); ++i) {
		ch = string[i];
		if (isspace(ch)) {
			space = true;
		} else if (space) {
			ch = toupper(ch);
			space = false;
		}
		buffer << ch;
	}
	out = buffer.str();
	return out;
}

std::string tostr(long num)
{
	StringBuffer str;
	str << num;
	return str.str();
}

long tolong(const std::string& str)
{
	return strtol(str.c_str(), NULL, 10);
}

// ----- std::string class stuff -----

std::string strip(const std::string& string)
{
	if (string.empty())
		return std::string();

	// strip back
	const char* back = string.c_str() + string.size() - 1;
	while (back >= string.c_str() && isspace(*back))
		--back;
	if (back < string.c_str())
		return std::string();

	// strip front
	const char* front = string.c_str();
	while (front < back && isspace(*front))
		++front;
	if (front == back)
		return std::string();

	// do substr
	return std::string(front, back - front + 1);
}

std::string strupper(const std::string& str)
{
	std::string rs = str;
	std::transform(str.begin(), str.end(), rs.begin(), toupper);
	return rs;
}

std::string strlower(const std::string& str)
{
	std::string rs = str;
	std::transform(str.begin(), str.end(), rs.begin(), tolower);
	return rs;
}

namespace {
	struct Replace {
		Replace (std::string s_from, const std::string& s_to) : from(s_from), to (s_to) {}

		const std::string& from;
		const std::string& to;

		char operator ()(char c) {
			char* r = strchr(from.c_str(), c);
			if (r != NULL)
				return to[(int)(from.c_str() - r)];
			return c;
		}
	};
}

std::string trim(const std::string& source, const std::string& accept)
{
	StringBuffer ret;

	for (const char* c = source.c_str(); *c != 0; ++c)
		if (strchr(accept.c_str(), *c))
			ret << *c;

	return ret.str();
}

std::string
str_tr (std::string source, const std::string& from, const std::string& to)
{
	StringBuffer result;

	for (const char* c = source.c_str(); *c != 0; ++c) {
		const char* l = std::strchr(from.c_str(), *c);
		if (l != NULL)
			result << to[l - from.c_str()];
		else
			result << *c;
	}

	return result.str();
}

std::string
make_path (const char* path, const char* file)
{
	assert(path != NULL);
	assert(file != NULL);

	// if the file begins with a /, it's already a full path
	if (file[0] == '/')
		return std::string(file);

	// if there is no path given, just return the file name
	if (path[0] == 0)
		return std::string(file);

	StringBuffer res;
	res << path;

	// if the path does not alrady end in a /, add one
	if (res[res.size()-1] != '/')
		res << '/';

	res << file;

	return res.str();
}

std::string
base_name (const char* path)
{
	assert(path != NULL);

	// find directory
	const char* start = strrchr(path, '/');
	if (start == NULL)
		start = path;
	else
		start = start + 1;

	// find last extension
	const char* ext = strrchr(start, '.');

	if (ext == NULL)
		return std::string(start);
	else
		return std::string(start, ext - start);
}

bool
has_suffix (const char* base, const char* suffix)
{
	assert(base != NULL);
	assert(suffix != NULL);

	if (strlen(base) < strlen(suffix))
		return false;

	if (strcmp(base + strlen(base) - strlen(suffix), suffix) == 0)
		return true;

	return false;
}
