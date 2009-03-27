/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_HTTP_H
#define SOURCEMUD_MUD_HTTP_H

#include "common/types.h"
#include "common/strbuf.h"
#include "common/streams.h"
#include "common/imanager.h"
#include "mud/account.h"
#include "net/netaddr.h"
#include "net/socket.h"

class HTTPHandler : public SocketConnection, public IStreamSink
{
public:
	HTTPHandler(int s_sock, const NetAddr& s_netaddr);

	// output
	virtual void streamPut(const char*, size_t len);
	virtual IStreamSink* getStream() { return this; }

	// processing
	void process();
	void execute();

	// hard-coded pages
	void pageIndex();
	void pageLogin();
	void pageLogout();
	void pageAccount();

	// error
	void httpError(int error);

	// get user account
	Account* getAccount() const { return account; }

	// low-level IO
	void disconnect();
	virtual void sockInput(char* buffer, size_t size);
	virtual void sockHangup();
	virtual void sockFlush();

	// log a request
	void log(int status);

	// serve a file
	void serve(const std::string& full_path);

	// get various values
	const std::string& getHeader(const std::string& name) const;
	const std::string& getCookie(const std::string& name) const;
	const std::string& getGET(const std::string& name) const;
	const std::string& getPOST(const std::string& name) const;
	const std::string& getRequest(const std::string& name) const;

protected:
	~HTTPHandler() {}

protected:
	// parse urlencoded data (GET/POST)
	void parseRequestData(std::map<std::string, std::string>& map, const char* input) const;

	NetAddr addr;

	// HTTP parsing
	StringBuffer line;
	std::string request;
	std::string method;
	std::string url;
	std::string path;
	enum { REQ, HEADER, BODY, DONE, ERROR } state;
	size_t content_length;
	time_t timeout;

	// request data
	std::map<std::string, std::string> header;
	std::map<std::string, std::string> get;
	std::map<std::string, std::string> post;
	std::map<std::string, std::string> cookie;

	// the session
	std::string session;
	Account* account;
};

class _HTTPManager : public IManager
{
public:
	virtual int initialize();
	virtual void shutdown();

	std::string getSessionKey() { return session_key; }

private:
	std::string session_key;
};
extern _HTTPManager HTTPManager;

#endif
