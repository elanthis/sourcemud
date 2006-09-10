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

typedef const char* CString;

/* hold a static string */
class StaticString {
	public:
	explicit StaticString (CString str) : string(str) {}

	CString string;
};

/* hold GC-allocated string */
class GCString {
	public:
	explicit GCString (CString str) : string(str) {}

	CString string;
};

/* hold a C++-style string in a less braind-dead way*/
class String : public GC
{
	public:
	String () : string("") {}
	String (const String& str) : string(str.string) {}
	explicit String (CString str);
	String (StaticString str) : string(str.string) {}
	String (GCString str) : string(str.string) {}
	String (CString str, size_t len);
	
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
	CString c_str () const { return string; }
	operator CString () const { return c_str(); }

	// size
	size_t size () const { return strlen(string); }

	// get a character
	const char operator[] (size_t index) const { return string[index]; }
	const char operator[] (int index) const { return string[index]; }

	// iterators
	typedef CString iterator;
	typedef CString const_iterator;
	iterator begin () const { return c_str(); }
	iterator end () const { return c_str() + size(); }

	// clear the string to zero
	void clear () { string = ""; }

	// get a substring
	String substr (size_t begin, size_t end) const;

	private:
	CString string;
};

// concatenation
String operator+ (String, String);
String operator+ (String, CString );
String operator+ (CString , String);

// comparison with C strings
inline bool operator== (String left, CString right) { return strcmp(left.c_str(), right) == 0; }
inline bool operator== (CString left, String right) { return strcmp(left, right.c_str()) == 0; }
inline bool operator!= (String left, CString right) { return strcmp(left.c_str(), right) != 0; }
inline bool operator!= (CString left, String right) { return strcmp(left, right.c_str()) != 0; }
inline bool operator< (String left, CString right) { return strcmp(left.c_str(), right) < 0; }
inline bool operator< (CString left, String right) { return strcmp(left, right.c_str()) < 0; }

// streams
inline std::ostream& operator << (std::ostream& stream, String str) { return stream << str.c_str(); }

// a list of strings
typedef GCType::vector<String> StringList;

// return suffix of number, like 1=>st, 2=>nd, etc.
String get_num_suffix (uint);

// conversion functions
String tostr (long num);
long tolong (String str);
String strupper (String str);
String strlower (String str);
String strip (String str);

// format check functions
bool str_is_number (String); // is a numeric value
bool str_is_email (String); // checks for semi-valid name@host.domain
bool str_is_true (String); // string is a "true" value
bool str_is_valid_id (String); // is a valid id
	// valid id defined as regex: [A-Za-z][A-Za-z0-9_.-]*

// various funcs
bool str_eq (String, String, size_t len = 0); // are strings equal, ignoring case - true if equal, len is max length or 0 for full

// match words in a phrase
// string to match is first, string being tested is second
// i.e., phrase_match("bob", str) will return true if str is all of or
// part of "bob"
bool phrase_match (CString haystack, CString needle);
// match a string prefix
// tests if chunk is a prefix of full; that is, "ret" is a prefix of "return",
// but "turn" is not.
bool prefix_match (CString string, CString prefix);

// determine a numeric value from input word, i.e. first=>1, second=>2, etc.
uint str_value (String);

// concatenate the path and filename together
//  if the path already ends in a /, don't add one
//  if the file begins with a /, ignore the path
String make_path (CString path, CString file);

// find the base name of the path; i.e., strip all directories and final suffix
// e.g.  /path/foo.html -> foo
String base_name (CString path);

// string list building/parsing
StringList& explode(StringList& out, String string, char ch);
String& implode(String& out, const StringList& list, char ch);
String& capwords(String& out, String string);
// and 'easy' versions there-of
inline StringList explode(String string, char ch) { StringList tmp; return explode(tmp, string, ch); }
inline String implode(const StringList& list, char ch) { String tmp; return implode(tmp, list, ch); }
inline String capwords(String string) { String tmp; return capwords(tmp, string); }

// trim out unacceptable characters
String trim (String source, String accept);

// replace characters in 'from' to corresponding characters in 'to'
String str_tr (String source, String from, String to);

#endif
