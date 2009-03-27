/*
 * Source MUD
 * Copyright(C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_NET_SOCKET_H
#define SOURCEMUD_NET_SOCKET_H

#include "common/types.h"
#include "common/strbuf.h"
#include "mud/server.h"

class ISocketHandler
{
public:
	virtual ~ISocketHandler() {}

	virtual void sockFlush() = 0;
	virtual void sockInReady() = 0;
	virtual void sockOutReady() = 0;
	virtual void sockHangup() = 0;

	virtual int sockGetFd() = 0;
	virtual bool sockIsOutWaiting() = 0;
	virtual bool sockIsDisconnectWaiting() = 0;
	virtual void sockCompleteDisconnect() = 0;
};

class SocketListener : public ISocketHandler
{
public:
	inline SocketListener(int s_sock) : sock(s_sock) {}

	// sub_classes provide sock_in_ready to accept() incoming connections
	virtual void sockInReady() = 0;

	int accept(class NetAddr& out) const;

private:
	// never need be called
	virtual void sockFlush() {}
	virtual void sockOutReady() {}
	virtual void sockHangup() {}

	virtual inline int sockGetFd() { return sock; }
	virtual bool sockIsOutWaiting() { return false; }
	virtual bool sockIsDisconnectWaiting() { return false; }
	virtual void sockCompleteDisconnect() {}

protected:
	int sock;
};

class SocketConnection : public ISocketHandler
{
public:
	SocketConnection(int s_sock);

	// called with input
	virtual void sockInput(char* buffer, size_t size) = 0;

	// sub-classes must implement:
	virtual void sockFlush() = 0;
	virtual void sockHangup() = 0;

	// request close
	void sockDisconnect();

	// add data to the output buffer
	void sockBuffer(const char* data, size_t size);

	// stats
	size_t getInBytes() const { return in_bytes; }
	size_t getOutBytes() const { return out_bytes; }

private:
	// internal
	virtual void sockInReady();
	virtual void sockOutReady();
	virtual int sockGetFd() { return sock; }
	virtual bool sockIsOutWaiting() { return !output.empty(); }
	virtual bool sockIsDisconnectWaiting() { return disconnect; }
	virtual void sockCompleteDisconnect();

private:
	std::vector<char> output;
	int sock;
	bool disconnect;
	size_t in_bytes;
	size_t out_bytes;
};

#endif
