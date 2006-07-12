/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

#include "common/string.h"
#include "common/error.h"
#include "common/streams.h"
#include "common/imanager.h"

enum LogClass {
	LOG_NOTICE,
	LOG_WARNING,
	LOG_ERROR,
	LOG_NETWORK,
	LOG_ADMIN
};

class SLogManager : public IManager
{
	public:
	int initialize (void);
	void shutdown (void);

	void print (LogClass klass, const char* msg);

	void reset (void);

	StringArg get_path (void) const { return path; }

	protected:
	String path;
	FILE *file;
};

extern SLogManager LogManager;

// C++ logging
namespace Log
{
	class LogWrapper : public IStreamSink
	{
		public:
		LogWrapper (LogClass s_klass) : klass(s_klass) {}

		virtual void stream_put (const char* str, size_t len) { msg = msg + String(str, len); }
		virtual bool stream_end (void);

		private:
		LogClass klass;
		String msg;
	};

	// the output
	extern LogWrapper Error;
	extern LogWrapper Warning;
	extern LogWrapper Info;
	extern LogWrapper Network;
	extern LogWrapper Admin;
}

#endif
