/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MUD_HTTP_H
#define AWEMUD_MUD_HTTP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "common/gcmap.h"
#include "common/types.h"
#include "common/gcbase.h"
#include "common/strbuf.h"
#include "common/streams.h"
#include "mud/network.h"
#include "common/imanager.h"
#include "scriptix/native.h"
#include "scriptix/function.h"
#include "mud/account.h"

class HTTPHandler : public Scriptix::Native, public SocketUser, public IStreamSink
{
	public:
	HTTPHandler (int s_sock, const SockStorage& s_netaddr);

	// output
	virtual void stream_put (const char*, size_t len);

	// processing
	void process ();
	void check_headers ();
	void execute ();

	// hard-coded pages
	void page_index ();
	void page_login ();
	void page_logout ();
	void page_account ();

	// get post data
	String get_post (String name) const;

	// get user account
	Account* get_account () const { return account; }

	// low-level IO
	void disconnect ();
	virtual void in_handle (char* buffer, size_t size);
	virtual char get_poll_flags ();
	virtual void out_ready ();
	virtual void hangup ();
	virtual void prepare ();

	protected:
	~HTTPHandler () {}

	protected:
	SockStorage addr;

	// HTTP parsing
	StringBuffer line;
	StringBuffer output;
	enum { GET, POST } reqtype;
	enum { REQ, HEADER, BODY, DONE, ERROR } state;
	String url;
	String reqid;
	GCType::map<String, String> headers;
	size_t content_length;

	// POST data
	GCType::map<String, String> post;

	// Registered account
	Account* account;
};

class SHTTPPageManager : public IManager
{
	public:
	virtual int initialize (void);
	virtual void shutdown (void);

	String get_template (String id);

	Scriptix::ScriptFunction get_page (String id);

	void register_page (String id, Scriptix::ScriptFunction func);

	private:
	typedef GCType::map<String, String> TemplateMap;
	TemplateMap templates;

	typedef GCType::map<String, Scriptix::ScriptFunction> PageMap;
	PageMap pages;
};
extern SHTTPPageManager HTTPPageManager;

#endif
