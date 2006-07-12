/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_COMMON_STRING_H
#define AWEMUD_COMMON_STRING_H

#include <string>
#include <sstream>

#include "gcbase.h"
#include "gcvector.h"
#include "types.h"

#define isvowel (ch) ((ch) == 'a' || (ch) == 'e' || (ch) == 'i' || (ch) == 'o' || (ch) == 'u' || (ch) == 'y')

#define S(str) String(StaticString(str))

/* hold a static string */
class StaticString {
	public:
	explicit StaticString (const char* str) : string(str) {}

	const char* string;
};

/* hold GC-allocated string */
class GCString {
	public:
	explicit GCString (const char* str) : string(str) {}

	const char* string;
};

/* hold a C++-style string in a less braind-dead way*/
class String : public GC
{
	public:
	String () : string("") {}
	String (const String& str) : string(str.string) {}
	explicit String (const char* str);
	String (StaticString str) : string(str.string) {}
	String (GCString str) : string(str.string) {}
	String (const char* str, size_t len);
	
	// copy operator
	String& operator = (const String &str) { string = str.string; return *this; }

	// comparison operators
	bool operator == (const String &str) const { return strcmp(string, str.string) == 0; }
	bool operator != (const String &str) const { return strcmp(string, str.string) != 0; }
	bool operator < (const String &str) const { return strcmp(string, str.string) < 0; }

	// empty test
	bool empty () const { return string[0] == 0; }
	operator bool () const { return !empty(); }

	// convert to c string
	const char* c_str () const { return string; }
	operator const char * () const { return c_str(); }

	// size
	size_t size () const { return strlen(string); }

	// get a character
	const char operator[] (size_t index) const { return string[index]; }
	const char operator[] (int index) const { return string[index]; }

	// iterators
	typedef const char* iterator;
	typedef const char* const_iterator;
	iterator begin () const { return c_str(); }
	iterator end () const { return c_str() + size(); }

	// clear the string to zero
	void clear () { string = ""; }

	// get a substring
	String substr (size_t begin, size_t end) const;

	private:
	const char* string;
};

// concatenation
String operator+ (String, String);
String operator+ (String, const char*);
String operator+ (const char*, String);

// comparison with C strings
inline bool operator== (String left, const char* right) { return strcmp(left.c_str(), right) == 0; }
inline bool operator== (const char* left, String right) { return strcmp(left, right.c_str()) == 0; }
inline bool operator!= (String left, const char* right) { return strcmp(left.c_str(), right) != 0; }
inline bool operator!= (const char* left, String right) { return strcmp(left, right.c_str()) != 0; }
inline bool operator< (String left, const char* right) { return strcmp(left.c_str(), right) < 0; }
inline bool operator< (const char* left, String right) { return strcmp(left, right.c_str()) < 0; }

// streams
inline std::ostream& operator << (std::ostream& stream, String str) { return stream << str.c_str(); }

typedef const String& StringArg;

typedef GCType::vector<String> StringList;

// return suffix of number, like 1=>st, 2=>nd, etc.
String get_num_suffix (uint);

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
bool str_eq (StringArg, StringArg, size_t len = 0); // are strings equal, ignoring case - true if equal, len is max length or 0 for full

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

#endif
