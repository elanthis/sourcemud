/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWE_STRING_H
#define AWE_STRING_H

#include <string>
#include <sstream>

#include "gcbase.h"
#include "gcvector.h"
#include "types.h"

#define isvowel (ch) ((ch) == 'a' || (ch) == 'e' || (ch) == 'i' || (ch) == 'o' || (ch) == 'u' || (ch) == 'y')

// GC'd base string
typedef std::basic_string<char, std::char_traits<char>, gc_allocator<char> > BaseString;

/* hold a C++-style string in a less braind-dead way*/
class String : public BaseString, public GC
{
	public:
	inline String (void) : BaseString() {}
	inline String (const String& str) : BaseString(str) {}
	inline String (const BaseString& str) : BaseString(str) {}
	inline String (const char* str) : BaseString() { if (str != NULL) BaseString::operator=(str); }
	inline String (const BaseString& str, size_t len) : BaseString(str, len) {}
	inline String (const char* str, size_t len) : BaseString() { append(str, len); }
	
	// copy operator
	inline String& operator = (const BaseString &str) { BaseString::operator=(str); return *this; }
	inline String& operator = (const String &str) { BaseString::operator=(str); return *this; }
	inline String& operator = (const char* str) { if (str == NULL) clear(); else BaseString::operator=(str); return *this; }

	// test operator
	inline operator bool (void) const { return !empty(); }
	// auto convert to c string
	inline operator const char * (void) const { return c_str(); }

	inline const char operator[] (size_t index) const { return BaseString::operator[](index); }
	inline const char operator[] (int index) const { return BaseString::operator[](index); }
};

typedef const String& StringArg;

typedef GCType::vector<String> StringList;

class StringBuffer : public std::basic_ostringstream<char, std::char_traits<char>, gc_allocator<char> >, public GC
{
	public:
	inline StringBuffer () {}
};

// return suffix of number, like 1=>st, 2=>nd, etc.
const char *get_num_suffix (uint);

// conversion functions
String tostr (long num);
long tolong (StringArg str);
String strupper (StringArg str);
String strlower (StringArg str);
String strip (StringArg str);

// format check functions
bool str_is_number (StringArg); // is a numeric value
bool str_is_email (StringArg); // checks for semi-valid name@host.domain
bool str_is_true (StringArg); // string is a "true" value
bool str_is_valid_id (StringArg); // is a valid id
	// valid id defined as regex: [A-Za-z][A-Za-z0-9_.-]*

// various funcs
bool str_eq (const char *, const char *, size_t len = 0); // are strings equal, ignoring case - true if equal, len is max length or 0 for full

// match words in a phrase
// string to match is first, string being tested is second
// i.e., phrase_match("bob", str) will return true if str is all of or
// part of "bob"
bool phrase_match (StringArg full, StringArg chunk);

// determine a numeric value from input word, i.e. first=>1, second=>2, etc.
uint str_value (StringArg);

// string list building/parsing
StringList& explode(StringList& out, StringArg string, char ch);
String& implode(String& out, const StringList& list, char ch);
String& capwords(String& out, StringArg string);
// and 'easy' versions there-of
inline StringList explode(StringArg string, char ch) { StringList tmp; return explode(tmp, string, ch); }
inline String implode(const StringList& list, char ch) { String tmp; return implode(tmp, list, ch); }
inline String capwords(StringArg string) { String tmp; return capwords(tmp, string); }

// trim out unacceptable characters
String trim (StringArg source, StringArg accept);

// replace characters in 'from' to corresponding characters in 'to'
String str_tr (StringArg source, StringArg from, StringArg to);

// search text lists
int get_index_of (const char** list, StringArg word, int start = -1);

// return true if string is true-value, false otherwise (AVOID USAGE - deprecated)
bool get_true_false (StringArg);

#endif
