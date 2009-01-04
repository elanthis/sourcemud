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

	virtual void sock_flush() = 0;
	virtual void sock_in_ready() = 0;
	virtual void sock_out_ready() = 0;
	virtual void sock_hangup() = 0;

	virtual int sock_get_fd() = 0;
	virtual bool sock_is_out_waiting() = 0;
	virtual bool sock_is_disconnect_waiting() = 0;
	virtual void sock_complete_disconnect() = 0;
};

class SocketListener : public ISocketHandler
{
public:
	inline SocketListener(int s_sock) : sock(s_sock) {}

	// sub_classes provide sock_in_ready to accept() incoming connections
	virtual void sock_in_ready() = 0;

	int accept(class NetAddr& out) const;

private:
	// never need be called
	virtual void sock_flush() {}
	virtual void sock_out_ready() {}
	virtual void sock_hangup() {}

	virtual inline int sock_get_fd() { return sock; }
	virtual bool sock_is_out_waiting() { return false; }
	virtual bool sock_is_disconnect_waiting() { return false; }
	virtual void sock_complete_disconnect() {}

protected:
	int sock;
};

class SocketConnection : public ISocketHandler
{
public:
	SocketConnection(int s_sock);

	// called with input
	virtual void sock_input(char* buffer, size_t size) = 0;

	// sub-classes must implement:
	virtual void sock_flush() = 0;
	virtual void sock_hangup() = 0;

	// request close
	void sock_disconnect();

	// add data to the output buffer
	void sock_buffer(const char* data, size_t size);

	// stats
	size_t get_in_bytes() const { return in_bytes; }
	size_t get_out_bytes() const { return out_bytes; }

private:
	// internal
	virtual void sock_in_ready();
	virtual void sock_out_ready();
	virtual int sock_get_fd() { return sock; }
	virtual bool sock_is_out_waiting() { return !output.empty(); }
	virtual bool sock_is_disconnect_waiting() { return disconnect; }
	virtual void sock_complete_disconnect();

private:
	std::vector<char> output;
	int sock;
	bool disconnect;
	size_t in_bytes;
	size_t out_bytes;
};

#endif
