/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/log.h"
#include "common/error.h"
#include "common/string.h"
#include "mud/server.h"
#include "mud/settings.h"

_MLog MLog;

namespace Log
{
	LogWrapper Error(LOG_ERROR);
	LogWrapper Warning(LOG_WARNING);
	LogWrapper Info(LOG_NOTICE);
	LogWrapper Network(LOG_NETWORK);
	LogWrapper Admin(LOG_ADMIN);

	void
	LogWrapper::stream_put(const char* text, size_t len)
	{
		msg.write(text, len);
	}

	void
	LogWrapper::stream_end()
	{
		MLog.print(klass, msg.str());
		msg.reset();
	}
}

int _MLog::initialize()
{
	path = MSettings.get_log_file();
	http_path = MSettings.get_http_log_file();

	if (!path.empty()) {
		if ((log = fopen(path.c_str(), "a")) == NULL) {
			Log::Error << "Unable to open log file " << path << ": " << strerror(errno);
			return 1;
		}
	}

	if (!http_path.empty()) {
		if ((http_log = fopen(http_path.c_str(), "a")) == NULL) {
			Log::Error << "Unable to open HTTP log file " << http_path << ": " << strerror(errno);
			return 1;
		}
	}

	Log::Info << "<--- Source MUD V" PACKAGE_VERSION " --->";

	return 0;
}

void _MLog::shutdown()
{
	if (log != NULL)
		fclose(log);
	if (http_log != NULL)
		fclose(http_log);
}

void _MLog::print(LogClass klass, const std::string& msg)
{
	// HTTP logs go to the HTTP log file, with no extra info
	if (klass == LOG_HTTP) {
		if (http_log)
			fprintf(http_log, "%s\n", msg.c_str());
		else if (log)
			fprintf(log, "%s\n", msg.c_str());
		else
			fprintf(stderr, "%s\n", msg.c_str());
		return;
	}

	char tbuf[41];
	const char* prefix;
	time_t t;
	struct tm local;
	time(&t);
	FILE* out = log ? log : stderr;

	switch (klass) {
	case LOG_ERROR:
		prefix = "**ERROR**: ";
		break;
	case LOG_WARNING:
		prefix = "[warning]: ";
		break;
	case LOG_NETWORK:
		prefix = "[network]: ";
		break;
	case LOG_ADMIN:
		prefix = "[admin]: ";
		break;
	case LOG_NOTICE:
	default:
		prefix = "";
		break;
	}

	strftime(tbuf, sizeof(tbuf) - 1, "%Y-%m-%d %T", localtime_r(&t, &local));
	fprintf(out, "%s - %s%s\n", tbuf, prefix, msg.c_str());
}

void _MLog::reset()
{
	if (log != NULL) {
		FILE* nlog = fopen(path.c_str(), "a");
		if (nlog == NULL) {
			Log::Warning << "Failed to re-load log file " << path << ": " << strerror(errno);
		} else {
			fclose(log);
			log = nlog;
		}
	}

	if (http_log != NULL) {
		FILE* nlog = fopen(http_path.c_str(), "a");
		if (nlog == NULL) {
			Log::Warning << "Failed to re-load HTTP log file " << http_path << ": " << strerror(errno);
		} else {
			fclose(http_log);
			http_log = nlog;
		}
	}
}
