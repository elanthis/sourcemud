/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_AWEREGEX_H
#define SOURCEMUD_COMMON_AWEREGEX_H 1

#include "common.h"

class RegEx {
	public:
	RegEx (const std::string& pattern, bool nocase = false);
	~RegEx ();

	bool grep (const std::string& string);
	std::vector<std::string> match (const std::string& string);

	private:
	regex_t regex;
};

#endif // __AWEREGEX_H__
