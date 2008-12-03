/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_STRING_H
#define SOURCEMUD_COMMON_STRING_H

#include "common.h"
#include "types.h"

#define isvowel (ch) ((ch) == 'a' || (ch) == 'e' || (ch) == 'i' || (ch) == 'o' || (ch) == 'u' || (ch) == 'y')

// return suffix of number, like 1=>st, 2=>nd, etc.
std::string get_num_suffix (uint);

// conversion functions
std::string tostr(long num);
long tolong(const std::string& str);
std::string strupper(const std::string& str);
std::string strlower(const std::string& str);
std::string strip(const std::string& str);

// format check functions
bool str_is_number(const std::string&); // is a numeric value
bool str_is_email(const std::string&); // checks for semi-valid name@host.domain
bool str_is_true(const std::string&); // string is a "true" value
bool str_is_false(const std::string&); // string is a "false" value
bool str_is_valid_id(const std::string&); // is a valid id
	// valid id defined as regex: [A-Za-z][A-Za-z0-9_.-]*

// various funcs
bool str_eq(const std::string&, const std::string&, size_t len = 0); // are strings equal, ignoring case - true if equal, len is max length or 0 for full

// match words in a phrase
// string to match is first, string being tested is second
// i.e., phrase_match("bob", str) will return true if str is all of or
// part of "bob"
bool phrase_match (const char* haystack, const char* needle);
inline bool phrase_match (const std::string& haystack, const char* needle) { return phrase_match(haystack.c_str(), needle); }
inline bool phrase_match (const std::string& haystack, const std::string& needle) { return phrase_match(haystack.c_str(), needle.c_str()); }
inline bool phrase_match (const char* haystack, const std::string& needle) { return phrase_match(haystack, needle.c_str()); }
// match a string prefix
// tests if chunk is a prefix of full; that is, "ret" is a prefix of "return",
// but "turn" is not.
bool prefix_match (const char* string, const char* prefix);
inline bool prefix_match (const char* string, const std::string& prefix) { return prefix_match(string, prefix.c_str()); }

// determine a numeric value from input word, i.e. first=>1, second=>2, etc.
uint str_value (const std::string&);

// concatenate the path and filename together
//  if the path already ends in a /, don't add one
//  if the file begins with a /, ignore the path
std::string make_path (const char* path, const char* file);

// find the base name of the path; i.e., strip all directories and final suffix
// e.g.  /path/foo.html -> foo
std::string base_name (const char* path);

// test if the suffix string is actually at the end of the base string
// useful for testing for file extensions and the like
bool has_suffix (const char* base, const char* suffix);

// string list building/parsing
std::vector<std::string>& explode(std::vector<std::string>& out, const std::string& string, char ch);
std::string& implode(std::string& out, const std::vector<std::string>& list, char ch);
std::string& capwords(std::string& out, const std::string& string);
// and 'easy' versions there-of
inline std::vector<std::string> explode(const std::string& string, char ch) { std::vector<std::string> tmp; return explode(tmp, string, ch); }
inline std::string implode(const std::vector<std::string>& list, char ch) { std::string tmp; return implode(tmp, list, ch); }
inline std::string capwords(const std::string& string) { std::string tmp; return capwords(tmp, string); }

// trim out unacceptable characters
std::string trim (const std::string& source, const std::string& accept);

// replace characters in 'from' to corresponding characters in 'to'
std::string str_tr (const std::string& source, const std::string& from, const std::string& to);

#endif
