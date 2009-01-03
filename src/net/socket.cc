/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/log.h"
#include "common/types.h"
#include "net/netaddr.h"
#include "net/socket.h"

int SocketListener::accept(NetAddr& addr) const
{
	// accept socket
	socklen_t sslen = sizeof(addr);
	int client = ::accept(sock, (struct sockaddr*) & addr, &sslen);
	if (client == -1)
		return -1;

	// set non-blocking flag
	fcntl(client, F_SETFL, O_NONBLOCK);
	return client;
}

SocketConnection::SocketConnection(int s_sock) : output(), sock(s_sock),
		disconnect(false), in_bytes(0), out_bytes(0)
{}

void
SocketConnection::sock_in_ready()
{
	char buffer[2048];
	int err = recv(sock, buffer, sizeof(buffer), 0);

	// fatal error
	if (err == -1 && errno != EAGAIN && errno != EINTR) {
		Log::Error << "recv() failed: " << strerror(errno);
		close(sock);
		sock = -1;

		sock_hangup();
		return;

		// eof
	} else if (err == 0) {
		close(sock);
		sock = -1;

		sock_hangup();
		return;

		// real data
	} else if (err > 0 && !disconnect) {
		in_bytes += err;
		sock_input(buffer, err);
	}
}


void
SocketConnection::sock_out_ready()
{
	int ret = send(sock, &output[0], output.size(), 0);
	if (ret > 0) {
		// HACK - this works, but isn't necessarily bright
		memmove(&output[0], &output[ret], output.size() - ret);
		output.resize(output.size() - ret);
	}

	// if we sent everything, clear buffer
	if (output.empty())
		output = std::vector<char>();
}

void
SocketConnection::sock_buffer(const char* bytes, size_t len)
{
	out_bytes += len;
	if (output.size() + len > output.capacity()) {
		// size is + 1024 bytes
		size_t newsize = ((output.size() + len) / 1024 + 1 * 1024);
		output.reserve(newsize);
	}
	size_t oldsize = output.size();
	output.resize(output.size() + len);
	memcpy(&output[oldsize], bytes, len);
}

void
SocketConnection::sock_disconnect()
{
	disconnect = true;
}

void
SocketConnection::sock_complete_disconnect()
{
	shutdown(sock, SHUT_RDWR);
	close(sock);
	sock = -1;
}
