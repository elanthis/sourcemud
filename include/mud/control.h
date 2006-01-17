/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef CONTROL_H
#define CONTROL_H

#include <unistd.h>

#include "network.h"
#include "server.h"
#include "imanager.h"
#include "streams.h"
#include "gcvector.h"

#define CONTROL_BUFFER_SIZE 1024

// a control user
class ControlHandler : public IStreamSink, public SocketUser
{
	public:
	ControlHandler (int s_sock, bool s_admin_flag);
	~ControlHandler (void) {}

	// output strings
	virtual void stream_put (const char* str, size_t len);

	// return true if the user can edit stuff
	inline bool is_admin (void) const { return admin_flag; }

	// network I/O
	virtual void prepare (void) {}
	virtual void in_handle (char* buf, size_t size);
	virtual char get_poll_flags (void);
	virtual void out_ready (void);
	virtual void hangup (void);

	private:
	char in_buffer[CONTROL_BUFFER_SIZE];
	String out_buffer;
	bool admin_flag;

	// parse input into command
	void process (void);

	// handle command
	void handle (int argc, char** argv);
};

// manage control users
class SControlManager : public IManager
{
	public:
	// initialize the control manager
	virtual int initialize (void);

	// shutdown the control manager
	virtual void shutdown (void);

	// return true if user has read-only access
	bool has_user_access (uid_t uid) const;
	
	// return true if user has admina ccess
	bool has_admin_access (uid_t uid) const;

	private:
	// users who have access
	GCType::vector<uid_t> user_list;
	GCType::vector<uid_t> admin_list;

	// yuck - let Control class manage their own membership
	friend class Control;
};
extern SControlManager ControlManager;

#endif
