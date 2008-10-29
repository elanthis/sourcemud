/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <netdb.h>
#if defined(HAVE_POLL)
#include <sys/poll.h>
#endif
#include <fcntl.h>

#include "common/log.h"
#include "common/types.h"
#include "mud/settings.h"
#include "mud/network.h"

#include <vector>

#include "config.h"

/********************
 * Misc Helper Code *
 ********************/

// apply mask to an IP address
// math/algorithm found in FreeBSD 'route' command - thanks guys!
namespace {
	void
	addr_apply_mask (uint8* addr, uint size, uint mask)
	{
		assert(mask <= size * 8);

		// buffer to hold mask
		uint8 maskbuf[size];
		memset(maskbuf, 0, size);

		// number of whole bytes
		uint q = mask >> 3;
		if (q > 0) memset(maskbuf, 0xff, q);
		
		// remainder bytes
		uint r = mask & 7;
		if (r > 0) *(maskbuf + q) = (0xff00 >> r) & 0xff;

		// apply to addr
		for (uint i = 0; i < size; ++i) 
			addr[i] &= maskbuf[i];
	}
}

// network addr parser
namespace {
	int
	parse_addr (const char* item, SockStorage* host, uint* mask)
	{
		char buffer[128];

		// put in buffer, snprintf() guarnatees NUL byte
		snprintf (buffer, sizeof(buffer), "%s", item);

		// get mask - have we a mask?
		int inmask = -1;
		char* slash = strchr(buffer, '/');
		if (slash != NULL) {
			*slash = '\0';
			++ slash;
			// don't use atoi or strtol, guarantee we parse it right
			inmask = 0;
			while (*slash != '\0') {
				if (!isdigit(*slash))
					break;
				inmask *= 10;
				inmask += *slash - '0';
				++ slash;
			}
			// only numbers, rights?
			if (*slash != '\0')
				return -1; // FAIL
		}

		// parse address
		SockStorage ss;
#ifdef HAVE_IPV6
		// try IPv6 first
		if (inet_pton(AF_INET6, buffer, &((sockaddr_in6*)&ss)->sin6_addr) > 0) { // match
			// check mask
			if (inmask > 128)
				return -1; // FAIL
			else if (inmask < 0)
				inmask = 128;
			addr_apply_mask((uint8*)&((sockaddr_in6*)&ss)->sin6_addr, 16, inmask);
			ss.ss_family = AF_INET6;
			*host = SockStorage(ss);
			*mask = inmask;
			return 0;
		} else
#endif // HAVE_IPV6
		// try IPv4 parsing
#ifdef HAVE_INET_PTON
		if (inet_pton(AF_INET, buffer, &((sockaddr_in*)&ss)->sin_addr) > 0) { // match
			// check mask
			if (inmask > 32)
				return -1; // FAIL
			else if (inmask < 0)
				inmask = 32;
			addr_apply_mask((uint8*)&((sockaddr_in*)&ss)->sin_addr, 4, inmask);
			ss.ss_family = AF_INET;
			*host = SockStorage(ss);
			*mask = inmask;
			return 0;
#else // HAVE_INET_PTON
		if (inet_aton(buffer, &((sockaddr_in*)&ss)->sin_addr) != 0) { // match
			// check mask
			if (inmask > 32)
				return -1; // FAIL
			else if (inmask < 0)
				inmask = 32;
			addr_apply_mask((uint8*)&((sockaddr_in*)&ss)->sin_addr, 4, inmask);
			ss.ss_family = AF_INET;
			*host = SockStorage(ss);
			*mask = inmask;
			return 0;
#endif
		} else {
			// no match
			return -1; // FAIL
		}
	}
}

/********************
 * SocketConnection *
 ********************/

SocketConnection::SocketConnection (int s_sock) : output(), sock(s_sock), disconnect(false)
{}

void
SocketConnection::sock_in_ready ()
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
	}

	// eof
	else if (err == 0) {
		close(sock);
		sock = -1;

		sock_hangup();
		return;
	}

	// real data
	else if (err > 0 && !disconnect)
		sock_input(buffer, err);
}


void
SocketConnection::sock_out_ready ()
{
	int ret = send(sock, &output[0], output.size(), 0);
	if (ret > 0) {
		// HACK - this works, but isn't necessarily bright
		memmove(&output[0], &output[ret], output.size() - ret);
		output.resize(output.size() - ret);
	}

	// if we sent everything, clear buffer
	if (output.empty())
		output = GCType::vector<char>();
}

void
SocketConnection::sock_buffer (const char* bytes, size_t len)
{
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
SocketConnection::sock_disconnect ()
{
	disconnect = true;
}

void
SocketConnection::sock_complete_disconnect ()
{
	shutdown(sock, SHUT_RDWR);
	close(sock);
	sock = -1;
}

/******************
 * SNetworkManager *
 ******************/

SNetworkManager NetworkManager;

struct PollData : public GC {
	GCType::vector<ISocketHandler*> sockets;
	GCType::vector<ISocketHandler*> addlist;
};

int
SNetworkManager::initialize ()
{
	p_data = new PollData();

	// set our hostname
	host = SettingsManager.get_hostname();
	if (!host) {
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
		host = String(host_info->h_name);
	}

	// load IP block list
	if (SettingsManager.get_deny_file()) {
		Log::Info << "Reading denied host list";

		if (denies.load(SettingsManager.get_deny_file()))
			return 1;
	}

	return 0;
}

void
SNetworkManager::shutdown ()
{
	p_data->sockets.resize(0);
	p_data->addlist.resize(0);
	delete p_data;
}

int
SNetworkManager::add_socket (ISocketHandler* socket)
{
	p_data->addlist.push_back(socket);
	return 0;
}

int
SNetworkManager::poll (long timeout)
{
	fd_set cread;
	fd_set cwrite;
	int max_sock = 0;

	FD_ZERO(&cread);
	FD_ZERO(&cwrite);

	GCType::vector<ISocketHandler*>::iterator i;

	// move the list of new sockets to the active socket list
	for (i = p_data->addlist.begin(); i != p_data->addlist.end(); ++i)
		p_data->sockets.push_back(*i);
	p_data->addlist.resize(0);

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

/**************
 * IPDenyList *
 **************/

int
IPDenyList::load (String path)
{
	char line[512];
	FILE* file;
	uint lcnt;
	
	file = fopen(path, "r");
	if (file == NULL) {
		Log::Error << "fopen() failed: " << path << ": " << strerror(errno);
		return -1;
	}

	lcnt = 0;
	while (fgets(line, sizeof(line), file) != NULL) {
		++lcnt;

		// chew off end of line
		for (char* c = line + strlen(line) - 1; c >= line; --c)
			if (isspace(*c))
				*c = 0;
			else
				break;

		// empty? skip
		if (!strlen(line))
			continue;

		// add and handle return codes
		int err = add(String(line));
		if (err == -1)
			Log::Warning << "Invalid IP deny line at " << path << ":" << lcnt;
	}

	fclose(file);
	return 0;
}

int
IPDenyList::save (String path)
{
	FILE* file;

	file = fopen(path, "w");
	if (file == NULL) {
		Log::Error << "fopen() failed: " << path << ": " << strerror(errno);
		return -1;
	}

	for (std::vector<IPDeny>::iterator i = denylist.begin(); i != denylist.end(); ++i)
		fprintf(file, "%s/%u\n", Network::get_addr_name(i->addr).c_str(), i->mask);

	fclose(file);

	return 0;
}

int
IPDenyList::remove (String line)
{
	SockStorage addr;
	uint mask;

	if (parse_addr(line, &addr, &mask))
		return -1;

	for (std::vector<IPDeny>::iterator i = denylist.begin(); i != denylist.end(); ++i)
		if (i->mask == mask && Network::addrcmp(addr, i->addr)) {
			denylist.erase(i);
			return 0;
		}

	return 1;
}

int
IPDenyList::add (String line)
{
	SockStorage addr;
	uint mask;

	if (parse_addr(line, &addr, &mask))
		return -1;

	for (uint i = 0; i < denylist.size(); ++i)
		if (Network::addrcmp(addr, denylist[i].addr))
			return 1;

	IPDeny deny;
	deny.addr = addr;
	deny.mask = mask;
	denylist.push_back(deny);
	return 0;
}

bool
IPDenyList::exists (SockStorage& addr)
{
	for (uint i = 0; i < denylist.size(); ++i)
		if (Network::addrcmp_mask(addr, denylist[i].addr, denylist[i].mask))
			return true;
	return false;
}

/**********************
 * Connection Tracker *
 **********************/

int
IPConnList::add (SockStorage& addr)
{
	// NOTE: we never limit connections from localhost

	// too many total connections?
	if (!Network::is_addr_local(addr) && total_conns >= SettingsManager.get_max_clients())
		return -1;

	// find existing connection from host
	for (std::vector<IPTrack>::iterator i = connections.begin(); i != connections.end(); ++i) {
		if (!Network::addrcmp(addr, i->addr)) {
			// too many from this host?
			if (!Network::is_addr_local(addr) && i->conns >= SettingsManager.get_max_per_host())
				return -2;

			// inc, and return OK
			++i->conns;
			return 0;
		}
	}

	// add new host
	IPTrack track;
	track.addr = addr;
	track.conns = 1;
	connections.push_back(track);
	return 0;
}

void
IPConnList::remove (SockStorage& addr)
{
	// find existing connection from host
	for (std::vector<IPTrack>::iterator i = connections.begin(); i != connections.end(); ++i) {
		if (!Network::addrcmp(addr, i->addr)) {
			// decrement
			--i->conns;
			--total_conns;

			// at zero?  remove
			if (i->conns == 0)
				connections.erase(i);

			return;
		}
	}
}

/*********************
 * Network Functions *
 *********************/

// compare addresses
int
Network::addrcmp (const SockStorage& addr1, const SockStorage& addr2)
{
	// same family, yes?
	if (addr1.ss_family != addr2.ss_family)
		return -1;

	switch (addr1.ss_family) {
		/* IPv4 */
		case AF_INET: {
			const sockaddr_in* sin1 = (sockaddr_in*)&addr1;
			const sockaddr_in* sin2 = (sockaddr_in*)&addr2;

			return *(uint32*)&sin1->sin_addr != *(uint32*)&sin2->sin_addr;
		}
		/* IPv6 */
#ifdef HAVE_IPV6
		case AF_INET6: {
			const sockaddr_in6* sin1 = (sockaddr_in6*)&addr1;
			const sockaddr_in6* sin2 = (sockaddr_in6*)&addr2;

			return !IN6_ARE_ADDR_EQUAL(&sin1->sin6_addr, &sin2->sin6_addr);
		}
#endif
		/* Unknown */
		default:
			return -1;
	}
}

// compare addresses - with mask applied to *first* address (only)
int
Network::addrcmp_mask (const SockStorage& in_addr1, const SockStorage& addr2, uint mask)
{
	// same family, yes?
	if (in_addr1.ss_family != addr2.ss_family)
		return -1;

	SockStorage addr1 = in_addr1;

	switch (addr1.ss_family) {
		/* IPv4 */
		case AF_INET: {
			sockaddr_in sin1 = *(sockaddr_in*)&addr1;
			const sockaddr_in sin2 = *(sockaddr_in*)&addr2;

			addr_apply_mask((uint8*)&sin1.sin_addr, sizeof(sin1.sin_addr), mask);

			return memcmp(&sin1.sin_addr, &sin2.sin_addr, sizeof(sin1.sin_addr));
		}
		/* IPv6 */
#ifdef HAVE_IPV6
		case AF_INET6: {
			sockaddr_in6 sin1 = *(sockaddr_in6*)&addr1;
			const sockaddr_in6 sin2 = *(sockaddr_in6*)&addr2;

			addr_apply_mask((uint8*)&sin1.sin6_addr, sizeof(sin1.sin6_addr), mask);

			return !IN6_ARE_ADDR_EQUAL(&sin1.sin6_addr, &sin2.sin6_addr);
		}
#endif
		/* Unknown */
		default:
			return -1;
	}
}

// get name of socket
String
Network::get_addr_name(const SockStorage& addr)
{
	char hostbuf[512];
	char servbuf[512];
	char buffer[512];

	// get the info
#if HAVE_GETNAMEINFO
	getnameinfo((sockaddr*)&addr, sizeof(addr), hostbuf, sizeof(hostbuf), servbuf, sizeof(servbuf), NI_NUMERICHOST | NI_NUMERICSERV);
#elif defined(HAVE_INET_PTON)
	struct sockaddr_in* sin = (struct sockaddr_in*)&addr;
	inet_ntop(AF_INET, &sin->sin_addr, hostbuf, sizeof(hostbuf));
	snprintf(servbuf, sizeof(servbuf), "%d", ntohs(sin->sin_port));
#else
	return "<unknown>";
#endif

	// no port?  just return the address
	if (!strlen(servbuf))
		return String(hostbuf);

	// host have a : (*cough* IPv6 *cough*) ? format [addr]:port
	if (strchr(hostbuf, ':'))
		snprintf(buffer, sizeof(buffer), "[%s]:%s", hostbuf, servbuf);
	else
		snprintf(buffer, sizeof(buffer), "%s:%s", hostbuf, servbuf);

	return String(buffer);
}

// get peer uid on AF_UNIX sockets
int
Network::get_peer_uid (int sock, uid_t& uid)
{
#if defined(HAVE_GETPEEREID)
	// use getpeereid
	uid_t gid;
	if (getpeereid(sock, &uid, &gid) != 0) {
		Log::Error << "getpeereid() failed: " << strerror(errno);
		return errno;
	}
	return 0;
#elif defined(SO_PEERCRED)
	// use Linux SO_PEERCRED getsockopt ability
	struct ucred peercred;
	socklen_t cred_len = sizeof(peercred);
	if (getsockopt(sock, SOL_SOCKET, SO_PEERCRED, &peercred, &cred_len)) {
		Log::Error << "getsockopt() failed: " << strerror(errno);
		return errno;
	}
	uid = peercred.uid;
	return 0;
#else
	// not supported - fail
	return EOPNOTSUPP;
#endif
}

// listen/server connection
int
Network::listen_tcp (int port, int family)
{
	SockStorage ss;
	size_t ss_len = sizeof(ss);
	int i_opt;
	int sock;

#ifdef HAVE_IPV6
	assert(family == AF_INET6 || family == AF_INET);
#else
	assert(family == AF_INET);
#endif // HAVE_IPV6

	// create socket
	if ((sock = socket(family, SOCK_STREAM, 0)) < 0) {
		Log::Error << "socket() failed: " << strerror(errno);
		return errno;
	}

	// set reuse timeout thingy
	i_opt = 1;
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &i_opt, sizeof (i_opt));

	// setup and config
#ifdef HAVE_IPV6
	if (family == AF_INET6) {
		// IPv6
		ss_len = sizeof(sockaddr_in6);
		ss.ss_family = AF_INET6;
		((sockaddr_in6*)&ss)->sin6_port = htons (port);
		memset (&((sockaddr_in6*)&ss)->sin6_addr, 0, sizeof (((sockaddr_in6*)&ss)->sin6_addr));

#ifdef IPV6_V6ONLY
		// set IPV6-only
		i_opt = 1;
		setsockopt (sock, SOL_SOCKET, IPV6_V6ONLY, &i_opt, sizeof (i_opt));
#endif
	} else
#endif // HAVE_IPV6
	if (family == AF_INET) {
		// IPv4
		ss_len = sizeof(sockaddr_in);
		ss.ss_family = AF_INET;
		((sockaddr_in*)&ss)->sin_port = htons (port);
		memset (&((sockaddr_in*)&ss)->sin_addr, 0, sizeof (((sockaddr_in*)&ss)->sin_addr));
	}

	// bind socket
	if (bind (sock, (struct sockaddr *)&ss, ss_len) == -1) {
		Log::Error << "bind() failed: " << strerror(errno);
		close(sock);
		return -1;;
	}

	// start listening
	if (::listen (sock, 5)) {
		Log::Error << "listen() failed: " << strerror(errno);
		close(sock);
		return -1;
	}

	return sock;
}

// accept incoming connections
int
Network::accept_tcp (int sock, SockStorage& addr)
{
	// accept socket
	socklen_t sslen = sizeof(addr);
	int client = accept(sock, (struct sockaddr*)&addr, &sslen);
	if (client == -1)
		return -1;

	// set non-blocking flag
	fcntl(client, F_SETFL, O_NONBLOCK);
	return client;
}


// true if address is local
bool
Network::is_addr_local (const SockStorage& addr)
{
	if (addr.ss_family == AF_INET) {
		if (((const sockaddr_in*)&addr)->sin_addr.s_addr == htonl(INADDR_LOOPBACK))
			return true;
	}
#ifdef HAVE_IPV6
	else if (addr.ss_family == AF_INET6) {
		if (IN6_IS_ADDR_LOOPBACK(&((const sockaddr_in6*)&addr)->sin6_addr))
			return true;
	}
#endif // HAVE_IPV6

	return false;
}

// listen/server connection
int
Network::listen_unix (String s_path)
{
	struct sockaddr_un address;
	size_t sa_len = sizeof(address);
	int sock;
	mode_t mode;

	// path too long?
	if (s_path.size() >= sizeof(address.sun_path)) {
		Log::Error << "UNIX socket path '" << s_path << " is too long (>" << sizeof(address.sun_path) << ")";
		return -1;
	}

	// create socket
	if ((sock = socket (PF_UNIX, SOCK_STREAM, 0)) < 0) {
		Log::Error << "socket() failed: " << strerror(errno);
		return -1;
	}

	// unlink previous socket
	unlink(s_path);

	// setup and config
	address.sun_family = AF_UNIX;
	snprintf(address.sun_path, sizeof(address.sun_path), "%s", s_path.c_str());
	sa_len = sizeof(address.sun_family) + strlen(address.sun_path) + 1;

	// bind socket
	mode = umask(077);
	if (bind (sock, (struct sockaddr *)&address, sa_len) == -1) {
		Log::Error << "bind() failed: " << strerror(errno);
		umask(mode);
		close(sock);
		return -1;
	}
	umask(mode);

	// start listening
	if (::listen (sock, 5)) {
		Log::Error << "listen() failed: " << strerror(errno);
		unlink(s_path);
		close(sock);
		return -1;
	}

	return sock;
}

// accept incoming connections
int
Network::accept_unix (int sock, uid_t& uid)
{
	// do accept
	struct sockaddr_un address;
	socklen_t sslen = sizeof(address);
	int client = accept(sock, (struct sockaddr*)&address, (socklen_t*)&sslen);
	if (client == -1)
		return -1;

	// get peer UID
	uid = 0;
	if (get_peer_uid(client, uid)) {
		close(client);
		return -1;
	}

	// set non-blocking flag
	fcntl(client, F_SETFL, O_NONBLOCK);

	return client;
}
