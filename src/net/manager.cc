/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/log.h"
#include "common/types.h"
#include "mud/settings.h"
#include "net/socket.h"
#include "net/manager.h"
#include "net/util.h"

_MNetwork MNetwork;

struct PollData {
	std::vector<ISocketHandler*> sockets;
	std::vector<ISocketHandler*> add;
};

int
_MNetwork::initialize ()
{
	p_data = new PollData();

	// set our hostname
	host = MSettings.get_hostname();
	if (host.empty()) {
		char host_buffer[256];
		if (gethostname(host_buffer, sizeof(host_buffer))) {
			Log::Error << "gethostname() failed: " << strerror(errno);
			return 1;
		}
		struct hostent* host_info = gethostbyname(host_buffer);
		if (host_info == NULL) {
			Log::Error << "gethostbyname() failed for " << host_buffer << ": " << strerror(errno);
			return 1;
		}
		host = std::string(host_info->h_name);
	}

	// load IP block list
	if (!MSettings.get_deny_file().empty()) {
		Log::Info << "Reading denied hosts from " << MSettings.get_deny_file();

		if (denies.load(MSettings.get_deny_file()))
			return 1;
	}

	return 0;
}

void
_MNetwork::shutdown ()
{
	for (std::vector<ISocketHandler*>::iterator i = p_data->sockets.begin(),
			e = p_data->sockets.end(); i != e; ++i)
		delete *i;
	p_data->sockets.clear();

	for (std::vector<ISocketHandler*>::iterator i = p_data->add.begin(),
			e = p_data->add.end(); i != e; ++i)
		delete *i;
	p_data->add.clear();

	delete p_data;
}

int
_MNetwork::add_socket (ISocketHandler* socket)
{
	p_data->add.push_back(socket);
	return 0;
}

int
_MNetwork::poll (long timeout)
{
	fd_set cread;
	fd_set cwrite;
	int max_sock = 0;

	FD_ZERO(&cread);
	FD_ZERO(&cwrite);

	std::vector<ISocketHandler*>::iterator i;

	// move the list of new sockets to the active socket list
	for (i = p_data->add.begin(); i != p_data->add.end(); ++i)
		p_data->sockets.push_back(*i);
	p_data->add.resize(0);

	// run prepare loop
	// build the cread and cwrite bit sets
	i = p_data->sockets.begin();
	while (i != p_data->sockets.end()) {
		if (!(*i)->sock_is_disconnect_waiting())
			(*i)->sock_flush();

		if ((*i)->sock_is_disconnect_waiting() && !(*i)->sock_is_out_waiting())
			(*i)->sock_complete_disconnect();

		int sock = (*i)->sock_get_fd();
		if (sock == -1) {
			delete *i;
			i = p_data->sockets.erase(i);
			continue;
		}
			
		if (sock > max_sock)
			max_sock = sock;

		FD_SET(sock, &cread);
		if ((*i)->sock_is_out_waiting())
			FD_SET(sock, &cwrite);

		++ i;
	}

	// convert timeout
	timeval tv;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	// do select
	errno = 0;
	int ret = select(max_sock + 1, &cread, &cwrite, NULL, timeout >= 0 ? &tv : NULL);

	// handle error
	if (ret == -1) {
		if (errno != EINTR)
			Log::Error << "select() failed: " << strerror(errno);
		return -1;
	}

	// process states
	if (ret > 0) {
		for (i = p_data->sockets.begin(); i != p_data->sockets.end(); ++i) {
			int sock = (*i)->sock_get_fd();
			if (FD_ISSET(sock, &cwrite))
				(*i)->sock_out_ready();
			if (FD_ISSET(sock, &cread))
				(*i)->sock_in_ready();
		}
	}

	return ret;
}
