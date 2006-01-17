/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef __AWEREGEX_H__
#define __AWEREGEX_H__ 1

#include <regex.h>

#include "gcbase.h"
#include "awestr.h"

class RegEx : public GCType::CleanupNonVirtual {
	public:
	RegEx (StringArg pattern, bool nocase = false);
	~RegEx ();

	bool grep (StringArg string);
	StringList match (StringArg string);

	private:
	regex_t regex;
};

#endif // __AWEREGEX_H__
