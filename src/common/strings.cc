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

String::String (const char* src)
{
	assert(src != NULL);
	char* tmp = (char*)GC_MALLOC(strlen(src) + 1);
	strcpy(tmp, src);
	string = tmp;
}

String::String (const char* src, size_t len)
{
	assert(src != NULL);
	char* tmp = (char*)GC_MALLOC(len + 1);
	strncpy(tmp, src, len);
	tmp[len] = 0;
	string = tmp;
}

String
operator+ (String left, String right)
{
	char* ret = (char*)GC_MALLOC(left.size() + right.size() + 1);
	strcpy(ret, left.c_str());
	strcpy(ret + left.size(), right.c_str());
	return GCString(ret);
}

String
operator+ (String left, const char* right)
{
	char* ret = (char*)GC_MALLOC(left.size() + strlen(right) + 1);
	strcpy(ret, left.c_str());
	strcpy(ret + left.size(), right);
	return GCString(ret);
}

String
operator+ (const char* left, String right)
{
	char* ret = (char*)GC_MALLOC(strlen(left) + right.size() + 1);
	strcpy(ret, left);
	strcpy(ret + strlen(left), right.c_str());
	return GCString(ret);
}

bool
str_is_valid_id (String string)
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
str_is_number (String string)
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
str_is_email (String string)
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
str_is_true (String string)
{
	return (
		str_eq(string, S("true")) ||
		str_eq(string, S("yes")) ||
		str_eq(string, S("on")) ||
		(str_is_number(string) && tolong(string) != 0)
	);
}

bool
str_eq (String str_a, String str_b, size_t len)
{
	// do comparison
	if (len)
		return !strncasecmp (str_a, str_b, len);
	else
		return !strcasecmp (str_a, str_b);
}

bool
prefix_match (CString string, CString prefix)
{
	assert(string != NULL);
	assert(prefix != NULL);

	return strncasecmp(string, prefix, strlen(prefix)) == 0;
}

bool
phrase_match (CString match, CString test)
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

String
get_num_suffix (unsigned int num) {
	if (num == 11 || num == 12 || num == 13) { return S("th"); }
	num %= 10;
	return num == 1 ? S("st") : num == 2 ? S("nd") : num == 3 ? S("rd") : S("th");
}

StringList&
explode (StringList& list, String str, char ch)
{
	list.clear();

	if (str.empty())
		return list;

	// break up str by the ch
	const char* l;
	const char* i;
	l = str.c_str();
	while ((i = strchr(l, ch)) != NULL) {
		list.push_back(String(l, i - l));;
		l = i + 1;
	}
	list.push_back(String(l));

	return list;
}

String&
implode (String& string, const StringList& list, char ch)
{
	StringBuffer buffer;

	// keep adding to string
	for (StringList::const_iterator i = list.begin(); i != list.end(); ++i) {
		if (!string.empty())
			buffer << ch;
		buffer << (*i).c_str();
	}

	return string = buffer.str();
}

String&
capwords (String& out, String string)
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

String
tostr (long num)
{
	StringBuffer str;
	str << num;
	return str.str();
}

long
tolong (String str)
{
	return strtol(str.c_str(), NULL, 10);
}

// ----- String class stuff -----

String
strip (String string)
{
	if (string.empty())
		return String();

	// strip back
	const char* back = string.c_str() + string.size() - 1;
	while (back >= string.c_str() && isspace(*back))
		--back;
	if (back < string.c_str())
		return String();

	// strip front
	const char* front = string.c_str();
	while (front < back && isspace(*front))
		++front;
	if (front == back)
		return String();

	// do substr
	return String(front, back - front + 1);
}

String
strupper (String string)
{
	char* ret = (char*)GC_MALLOC(string.size() + 1);
	for (size_t i = 0; i < string.size() + 1; ++i)
		ret[i] = toupper(string[i]);
	return GCString(ret);
}

String
strlower (String string)
{
	char* ret = (char*)GC_MALLOC(string.size() + 1);
	for (size_t i = 0; i < string.size() + 1; ++i)
		ret[i] = tolower(string[i]);
	return GCString(ret);
}

namespace {
	struct Replace {
		Replace (String s_from, String s_to) : from(s_from), to (s_to) {}

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
trim (String source, String accept)
{
	StringBuffer ret;

	for (const char* c = source.c_str(); *c != 0; ++c)
		if (strchr(accept.c_str(), *c))
			ret << *c;

	return ret.str();
}

String
str_tr (String source, String from, String to)
{
	StringBuffer result;

	for (const char* c = source.c_str(); *c != 0; ++c) {
		const char* l = strchr(from, *c);
		if (l != NULL)
			result << to[l - from.c_str()];
		else
			result << *c;
	}

	return result.str();
}

String
make_path (CString path, CString file)
{
	assert(path != NULL);
	assert(file != NULL);

	// if the file begins with a /, it's already a full path
	if (file[0] == '/')
		return String(file);

	// if there is no path given, just return the file name
	if (path[0] == 0)
		return String(file);

	StringBuffer res;
	res << path;

	// if the path does not alrady end in a /, add one
	if (res[res.size()-1] != '/')
		res << '/';

	res << file;

	return res.str();
}

String
base_name (CString path)
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
		return String(start);
	else
		return String(start, ext - start);
}
