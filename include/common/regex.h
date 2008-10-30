/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_AWEREGEX_H
#define SOURCEMUD_COMMON_AWEREGEX_H 1

#include <sys/types.h>
#include <regex.h>

#include "common/gcbase.h"
#include "common/string.h"

class RegEx : public GCType::CleanupNonVirtual {
	public:
	RegEx (String pattern, bool nocase = false);
	~RegEx ();

	bool grep (String string);
	StringList match (String string);

	private:
	regex_t regex;
};

#endif // __AWEREGEX_H__
