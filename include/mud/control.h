/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef CONTROL_H
#define CONTROL_H

#include <sys/types.h>
#include <unistd.h>

#include "mud/network.h"
#include "mud/server.h"
#include "common/imanager.h"
#include "common/streams.h"
#include "common/gcvector.h"
#include "mud/account.h"

#define CONTROL_BUFFER_SIZE 1024

// a control user
class ControlHandler : public IStreamSink, public SocketUser
{
	public:
	ControlHandler (int s_sock, uid_t s_uid);
	~ControlHandler () {}

	// output strings
	virtual void stream_put (const char* str, size_t len);

	// return true if the user can edit stuff
	bool is_admin () const;

	// network I/O
	virtual void prepare () {}
	virtual void in_handle (char* buf, size_t size);
	virtual char get_poll_flags ();
	virtual void out_ready ();
	virtual void hangup ();

	private:
	char in_buffer[CONTROL_BUFFER_SIZE];
	String out_buffer;
	Account* account;
	uid_t uid;

	// parse input into command
	void process ();

	// handle command
	void handle (int argc, String argv[]);
};

// manage control users
class SControlManager : public IManager
{
	public:
	// initialize the control manager
	virtual int initialize ();

	// shutdown the control manager
	virtual void shutdown ();

	// return true if the user id may use the control interface
	bool has_user_access (uid_t uid) const;
	
	// return true if user has admin access
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
