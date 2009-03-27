/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef __SOURCEMUD_COMMON_LOG_H__
#define __SOURCEMUD_COMMON_LOG_H__ 1

#include "common/error.h"
#include "common/streams.h"
#include "common/imanager.h"

enum LogClass {
	LOG_NOTICE,
	LOG_WARNING,
	LOG_ERROR,
	LOG_NETWORK,
	LOG_HTTP,
	LOG_ADMIN
};

class _MLog : public IManager
{
public:
	_MLog() : log(NULL), http_log(NULL) {}

	int initialize();
	void shutdown();

	void print(LogClass klass, const std::string& msg);

	void reset();

	std::string getPath() const { return path; }
	std::string getHttpPath() const { return http_path; }

protected:
	std::string path;
	std::string http_path;
	FILE* log;
	FILE* http_log;
};

extern _MLog MLog;

// C++ logging
namespace Log
{
class LogWrapper : public IStreamSink
{
public:
	LogWrapper(LogClass s_klass) : klass(s_klass) {}

	virtual void streamPut(const char* str, size_t len);
	virtual void streamEnd();

private:
	LogClass klass;
	std::ostringstream msg;
};

// the output
extern LogWrapper Error;
extern LogWrapper Warning;
extern LogWrapper Info;
extern LogWrapper Network;
extern LogWrapper Admin;
}

#endif
