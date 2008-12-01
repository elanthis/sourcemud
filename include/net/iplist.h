/*
 * Source MUD
 * Copyright(C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_NET_IPLIST_H 
#define SOURCEMUD_NET_IPLIST_H 

class IPDenyList {
	public:
	int load(const std::string& file);
	int save(const std::string& file);

	// these return -1 on invalid input, 1 on exist errors, 0 on success
	int add(const std::string& addr);
	int remove(const std::string& addr);

	bool exists(SockStorage& addr);

	private:
	struct IPDeny {
		SockStorage addr;
		uint mask;
	};
	std::vector<IPDeny> denylist;
};

class IPConnList {
	public:
	struct IPTrack {
		SockStorage addr;
		int conns;
	};
	typedef std::vector<IPTrack> ConnList;

	// return -1 if at max total users, -2 if max per host, 0 on success
	int add(SockStorage& addr);
	void remove(SockStorage& addr);

	const ConnList& get_conn_list() const { return connections; }
	inline uint get_total() { return total_conns; }

	private:
	ConnList connections;
	int total_conns;
};

#endif
