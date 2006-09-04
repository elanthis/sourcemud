/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#define HTTP_REQUEST_TIMEOUT 30 // 30 seconds
#define HTTP_HEADER_LINE_MAX 2048 // arbitrary max line length
#define HTTP_POST_BODY_MAX (16*1024) // 16K

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdarg.h>

#include "common/error.h"
#include "mud/server.h"
#include "mud/network.h"
#include "mud/parse.h"
#include "common/streams.h"
#include "mud/http.h"
#include "mud/settings.h"
#include "common/log.h"
#include "mud/fileobj.h"
#include "common/md5.h"

SHTTPPageManager HTTPPageManager;

SCRIPT_TYPE(HTTP);
HTTPHandler::HTTPHandler (int s_sock, const SockStorage& s_netaddr) : Scriptix::Native(AweMUD_HTTPType), SocketUser(s_sock)
{
	addr = s_netaddr;
	state = REQ;
	session = NULL;
	timeout = time(NULL);
}

// disconnect
void
HTTPHandler::disconnect ()
{
	// don't log this; FIXME: we should ahve one log line per request, noting usrl/user/etc
	// Log::Network << "HTTP client disconnected: " << Network::get_addr_name(addr);

	// reduce count
	NetworkManager.connections.remove(addr);

	// close socket
	if (sock != -1) {
		close(sock);
		sock = -1;
	}
}

/* output a data of text -
 * deal with formatting new-lines and such, and also
 * escaping/removing/translating AweMUD commands
 */
void
HTTPHandler::stream_put (const char *text, size_t len) 
{
	output.write(text, len);
}

// process input
void
HTTPHandler::in_handle (char* buffer, size_t size)
{
	timeout = time(NULL);

	while (size > 0) {
		// if we're in the body of a POST, we need to get content_length bytes
		if (state == BODY) {
			size_t remain = content_length - line.size();
			if (size >= remain) {
				line.write(buffer, remain);
				size -= remain;
				buffer += remain;
				if (line.size() >= content_length) {
					process();
					line.clear();
				}
			} else {
				line.write(buffer, size);
				size = 0;
			}
		// if we're in REQ or HEADER, we need to read in lines
		} else if (state == REQ || state == HEADER) {
			char* c;
			if ((c = strchr(buffer, '\n')) != NULL) {
				// put line into buffer; hack to ignore \r
				if (c > buffer && *(c - 1) == '\r')
					line.write(buffer, c - buffer - 1);
				else
					line.write(buffer, c - buffer);
				// process the line
				process();
				line.clear();
				// update input data
				size -= (c - buffer) + 1;
				buffer = c + 1;
			} else {
				// if we still don't have a line but we're over the max, fail
				if (line.size() + size >= HTTP_HEADER_LINE_MAX) {
					Log::Network << "HTTP - error ? " << Network::get_addr_name(addr) << " <header length overflow>";
					*this << "HTTP/1.0 413 Request Entity Too Large\n\nRequest header length overflow.\n";
				}

				// append remaining data to our line buffer
				line.write(buffer, size);
			}
		// other states; just ignore any further data
		} else if (state == DONE || state == ERROR) {
			break;
		}
	}
}

// flush out the output, write prompt
void
HTTPHandler::prepare ()
{
	if (timeout != 0 && (time(NULL) - timeout) >= HTTP_REQUEST_TIMEOUT) {
		Log::Network << "HTTP - error - " << Network::get_addr_name(addr) << " <request timeout>";
		*this << "HTTP/1.0 408 Request Timeout\n\nRequest timed out.\n";
		state = ERROR;
		timeout = 0;
	}
}

char
HTTPHandler::get_poll_flags ()
{
	char flags = POLLSYS_READ;
	if (!output.empty())
		flags |= POLLSYS_WRITE;
	return flags;
}

void
HTTPHandler::out_ready ()
{
	// FIXME: do this right
	send(sock, output.c_str(), output.size(), 0);
	output.clear();

	// disconnect if that we are done
	if (state == DONE || state == ERROR)
		disconnect();
}

void
HTTPHandler::hangup ()
{
	disconnect();
}

void
HTTPHandler::process ()
{
	switch (state) {
		case REQ:
		{
			// empty line?  ignore
			if (line.empty())
				return;

			// parse
			StringList parts = explode(line.str(), ' ');

			// check size
			if (parts.size() != 3) {
				Log::Network << "HTTP - error ? " << Network::get_addr_name(addr) << " <malformed request>";
				*this << "HTTP/1.0 400 Bad Request\n\nInvalid request\n";
				state = ERROR;
				return;
			}

			// request type
			if (parts[0] == "GET")
				reqtype = GET;
			else if (parts[0] == "POST")
				reqtype = POST;
			else {
				Log::Network << "HTTP " << parts[0] << " " << parts[1] << " error ? " << Network::get_addr_name(addr) << " <illegal request type>";
				*this << "HTTP/1.0 405 Method Not Allowed\n\nUnauthorized request type " << parts[0] << "\n";
				state = ERROR;
				return;
			}

			// get URL
			url = parts[1];

			state = HEADER;
			break;
		}
		case HEADER:
		{
			// no more headers
			if (line.empty()) {
				// do header check
				check_headers();
				if (state == ERROR)
					return;

				// a GET request is now processed immediately
				if (reqtype == GET) {
					execute();
				// a POST request requires us to parse the body first
				} else {
					state = BODY;
				}
				break;
			}

			// parse the header
			const char* c = strchr(line.c_str(), ':');
			// require ': ' after header name
			if (c == NULL || *(c + 1) != ' ') {
				Log::Network << "HTTP " << (reqtype == GET ? "GET" : "POST") << " " << url << " error ? " << Network::get_addr_name(addr) << " <malformed header>";
				*this << "HTTP/1.0 400 Bad Request\n\nInvalid header:\n" << line.c_str() << "\n";
				state = ERROR;
				return;
			}
			headers[strlower(String(line.c_str(), c - line.c_str()))] = String(c + 2);
			break;
		}
		case BODY:
		{
			// parse the data
			StringBuffer value;
			const char* begin;
			const char* end;
			const char* sep;

			begin = line.c_str();
			do {
				// find the end of the current pair
				end = strchr(begin, '&');
				if (end == NULL)
					end = line.c_str() + line.size();

				// find the = separator
				sep = strchr(begin, '=');
				if (sep == NULL || sep > end) {
					Log::Network << "HTTP " << (reqtype == GET ? "GET" : "POST") << " " << url << " error ? " << Network::get_addr_name(addr) << " <malformed form data>";
					*this << "HTTP/1.0 400 Bad Request\n\nMalformed form data.\n";
					state = ERROR;
					return;
				}

				// decode the value
				value.clear();
				for (const char* c = sep + 1; c < end; ++c) {
					if (*c == '+') {
						value << ' ';
					} else if (*c == '%') {
						int r;
						sscanf(c + 1, "%2X", &r);
						value << (char)r;
						c += 2;
					} else {
						value << *c;
					}
				}

				// store the data
				post[strlower(String(begin, sep - begin))] = value.str();

				// set begin to next token
				begin = end;
				if (*begin == '&')
					++begin;
			} while (*begin != 0);

			// execute
			execute();
			break;
		}
		case DONE:
		case ERROR:
			break;
	}
}

void
HTTPHandler::check_headers()
{
	// if we're a POST, we must have the proper headers/values
	if (reqtype == POST) {
		// must be application/x-www-form-urlencoded
		if (headers[S("content-type")] != "application/x-www-form-urlencoded") {
			Log::Network << "HTTP " << (reqtype == GET ? "GET" : "POST") << " " << url << " error ? " << Network::get_addr_name(addr) << " <incorrect content-type>";
			*this << "HTTP/1.0 406 Not Acceptable\n\nContent-type must be application/x-www-form-urlencoded.\n";
			state = ERROR;
			return;
		}

		// must have a content-length
		String len = headers[S("content-length")];
		if (len.empty()) {
			Log::Network << "HTTP " << (reqtype == GET ? "GET" : "POST") << " " << url << " error ? " << Network::get_addr_name(addr) << " <missing content-length>";
			*this << "HTTP/1.0 406 Not Acceptable\n\nNo Content-length was provided.\n";
			state = ERROR;
			return;
		}
		content_length = strtoul(len.c_str(), NULL, 10);

		// check max content length
		if (content_length > HTTP_POST_BODY_MAX) {
			Log::Network << "HTTP " << url << " error " << (get_account() ? get_account()->get_id() : S("-")) << Network::get_addr_name(addr) << " <form data overflow>";
			*this << "HTTP/1.0 413 Request Entity Too Large\n\nForm data length overflow.\n";
		}
	}

	// if we have a session id, use it
	String cookies = headers[S("cookie")];
	const char* sid_start;
	if ((sid_start = strstr(cookies.c_str(), "AWEMUD_SESSION=")) != NULL) {
		sid_start = strchr(sid_start, '=') + 1;
		const char* sid_end = strchr(sid_start, ';');
		String sid;
		if (sid_end == NULL)
			sid = String(sid_start);
		else
			sid = String(sid_start, sid_end - sid_start);

		// get the session (may be NULL if expired/invalid)
		session = HTTPPageManager.get_session(sid);
		if (session != NULL)
			session->update_timestamp();
	}
}

void
HTTPHandler::execute()
{
	// split off reqid if we have one
	String path = url;
	const char* sep = strchr(url, '?');
	String reqid;
	if (sep != NULL) {
		path = String(url, sep - url.c_str());
		reqid = String(sep + 1);
	}

	// handle built-in pages
	if (path == "/")
		page_index();
	else if (path == "/login")
		page_login();
	else if (path == "/logout")
		page_logout();
	else if (path == "/account")
		page_account();
	// do a script page
	else {
		Scriptix::ScriptFunction func = HTTPPageManager.get_page(path);
		if (func) {
			func.run(path, reqid, this);
		} else {
			Log::Network << "HTTP " << (reqtype == GET ? "GET" : "POST") << " " << url << " error " << (get_account() ? get_account()->get_id() : S("-")) << " " << Network::get_addr_name(addr) << " <page not found>";
			*this << "HTTP/1.0 404 Not Found\n\nPage not found\n\n";
			state = ERROR;
			return;
		}
	}

	// log access
	Log::Network << "HTTP " << (reqtype == GET ? "GET" : "POST") << " " << url << " valid " << (get_account() ? get_account()->get_id() : S("-")) << " " << Network::get_addr_name(addr);


	// done
	state = DONE;
}

void
HTTPHandler::page_index()
{
	*this << "HTTP/1.0 200 OK\nContent-type: text/html\n\n"
		<< StreamParse(HTTPPageManager.get_template(S("header")), S("account"), get_account())
		<< StreamParse(HTTPPageManager.get_template(S("index")), S("account"), get_account())
		<< StreamParse(HTTPPageManager.get_template(S("footer")));
}

void
HTTPHandler::page_login()
{
	String msg;

	*this << "HTTP/1.0 200 OK\nContent-type: text/html\n";

	// attempt login?
	if (post[S("action")] == "Login") {
		Account* account = AccountManager.get(post[S("username")]);
		if (account != NULL && account->check_passphrase(post[S("password")])) {
			session = HTTPPageManager.create_session(account);
			msg = S("Login successful!");
			*this << "Set-cookie: AWEMUD_SESSION=" << session->get_id() << "\n";
		} else {
			msg = S("Incorrect username or passphrase.");
		}
	}

	*this << "\n"
		<< StreamParse(HTTPPageManager.get_template(S("header")), S("msg"), msg, S("account"), get_account())
		<< StreamParse(HTTPPageManager.get_template(S("login")), S("account"), get_account())
		<< StreamParse(HTTPPageManager.get_template(S("footer")));
}

void
HTTPHandler::page_logout()
{
	if (session != NULL)
		HTTPPageManager.destroy_session(session);

	*this << "HTTP/1.0 200 OK\nContent-type: text/html\nSet-cookie: AWEMUD_SESSION=\n\n"
		<< StreamParse(HTTPPageManager.get_template(S("header")))
		<< StreamParse(HTTPPageManager.get_template(S("logout")))
		<< StreamParse(HTTPPageManager.get_template(S("footer")));
}

void
HTTPHandler::page_account()
{
	// must have an account for this to work
	if (get_account() == NULL) {
		*this << "HTTP/1.0 403 Access Denied\n\nYou must login to access this page.\n";
		return;
	}

	String msg;
	if (post[S("action")] == "Save Changes") {
		String name = strip(post[S("acct_name")]);
		String email = strip(post[S("acct_email")]);
		if (name.empty() || email.empty()) {
			msg = S("You may not enter an empty name or email address.");
		} else {
			String pass1 = post[S("new_pass1")];
			String pass2 = post[S("new_pass2")];

			if (pass1 != pass2) {
				msg = S("Passphrases do not match.");
			} else {
				if (!pass1.empty())
					get_account()->set_passphrase(pass1);

				get_account()->set_name(name);
				get_account()->set_email(email);
				get_account()->save();

				msg = S("Changes saved successfully!");
			}
		}
	}

	// did they try to save changes?
	*this << "HTTP/1.0 200 OK\nContent-type: text/html\n\n"
		<< StreamParse(HTTPPageManager.get_template(S("header")), S("account"), get_account(), S("msg"), msg)
		<< StreamParse(HTTPPageManager.get_template(S("account")), S("account"), get_account())
		<< StreamParse(HTTPPageManager.get_template(S("footer")));
}

String
HTTPHandler::get_post (String id) const
{
	GCType::map<String,String>::const_iterator i = post.find(id);
	if (i == post.end())
		return String();
	return i->second;
}

int
HTTPHandler::parse_property (const StreamControl& stream, String method, const ParseArgs& argv) const
{
	if (method == "post" && argv.size() == 1) {
		stream << get_post(argv[0].get_string());
		return 0;
	} else {
		return -1;
	}
}

void
HTTPHandler::parse_default (const StreamControl& stream) const
{
	stream << "<HTTP>";
}

HTTPSession::HTTPSession (Account* account)
{
	assert(account != NULL);

	// initial setup
	this->account = account;
	timestamp = time(NULL);

	// generate session ID
	StringBuffer buf;
	buf << "AweMUD";
	buf << account->get_id();
	buf << account->get_name();
	buf << rand();
	buf << account->get_email();
	buf << timestamp;
	char md5buf[MD5_BUFFER_SIZE];
	MD5::hash(buf.c_str(), md5buf);
	id = String(md5buf);
}

void
HTTPSession::update_timestamp ()
{
	timestamp = time(NULL);
}

bool
HTTPSession::check_timestamp ()
{
	return (time(NULL) - timestamp) < SettingsManager.get_http_timeout();
}

String
HTTPSession::get_var (String id) const
{
	GCType::map<String,String>::const_iterator i = vars.find(id);
	if (i == vars.end())
		return String();
	return i->second;
}

void
HTTPSession::set_var (String id, String val)
{
	vars[id] = val;
}

void
HTTPSession::clear ()
{
	vars.clear();
	id.clear();
	account = NULL;
}

int
SHTTPPageManager::initialize (void)
{
	File::Reader reader;

	// open templates file
	if (reader.open(SettingsManager.get_misc_path() + "/html"))
		return -1;

	// read said file
	FO_READ_BEGIN
		FO_WILD("html")
			templates[node.get_key()] = node.get_data();
	FO_READ_ERROR
		// damnable errors!
		return -1;
	FO_READ_END

	// all good
	return 0;
}

void
SHTTPPageManager::shutdown (void)
{
	templates.clear();
}

String
SHTTPPageManager::get_template (String id)
{
	TemplateMap::iterator i = templates.find(id);
	return i != templates.end() ? i->second : String();
}

Scriptix::ScriptFunction
SHTTPPageManager::get_page (String id)
{
	PageMap::iterator i = pages.find(id);
	return i != pages.end() ? i->second : Scriptix::ScriptFunction();
}

void
SHTTPPageManager::register_page (String id, Scriptix::ScriptFunction func)
{
	pages[id] = func;
}

HTTPSession*
SHTTPPageManager::create_session (Account* account)
{
	assert(account != NULL);

	HTTPSession* session = new HTTPSession(account);
	sessions[session->get_id()] = session;

	return session;
}

void
SHTTPPageManager::destroy_session (HTTPSession* session)
{
	assert(session != NULL);

	SessionMap::iterator i = sessions.find(session->get_id());
	if (i != sessions.end())
		sessions.erase(i);
	session->clear();
	session = NULL;
}

HTTPSession*
SHTTPPageManager::get_session (String id)
{
	SessionMap::iterator i = sessions.find(id);
	if (i == sessions.end())
		return NULL;
	return i->second;
}

void
SHTTPPageManager::check_timeouts ()
{
	SessionMap::iterator i, n;
	i = sessions.begin();
	while (i != sessions.end()) {
		n = i;
		++n;
		if (!i->second->check_timestamp()) {
			i->second->clear();
			sessions.erase(i);
		}
		i = n;
	}
}
