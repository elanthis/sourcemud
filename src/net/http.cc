/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#define HTTP_REQUEST_TIMEOUT 30 // 30 seconds
#define HTTP_HEADER_LINE_MAX 2048 // arbitrary max line length
#define HTTP_POST_BODY_MAX (16*1024) // 16K

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include "common/error.h"
#include "common/streams.h"
#include "common/log.h"
#include "common/md5.h"
#include "common/file.h"
#include "mud/server.h"
#include "mud/macro.h"
#include "mud/settings.h"
#include "net/http.h"
#include "net/manager.h"
#include "net/util.h"

#include "config.h"

SHTTPManager HTTPManager;

HTTPHandler::HTTPHandler (int s_sock, const SockStorage& s_netaddr) :  SocketConnection(s_sock)
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
	// reduce count
	NetworkManager.connections.remove(addr);

	// close socket
	sock_disconnect();
}

/* output a data of text -
 * deal with formatting new-lines and such, and also
 * escaping/removing/translating Source MUD commands
 */
void
HTTPHandler::stream_put (const char *text, size_t len) 
{
	sock_buffer(text, len);
}

// process input
void
HTTPHandler::sock_input (char* buffer, size_t size)
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
					http_error(413, S("Header too large."));
					return;
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
HTTPHandler::sock_flush ()
{
	// handle timeout
	if (timeout != 0 && (time(NULL) - timeout) >= HTTP_REQUEST_TIMEOUT) {
		http_error(408, S("Request timed out."));
		timeout = 0;
	}

	// disconnect if we are all done
	if (state == DONE || state == ERROR)
		sock_disconnect();
}

void
HTTPHandler::sock_hangup ()
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
				http_error(400, S("Invalid request."));
				return;
			}

			// request type
			if (parts[0] == "GET")
				reqtype = GET;
			else if (parts[0] == "POST")
				reqtype = POST;
			else {
				http_error(405, S("Illegal request type."));
				return;
			}

			// get URL
			url = parts[1];
			const char* sep = strchr(url.c_str(), '?');
			if (sep != NULL) {
				path = std::string(url.c_str(), sep - url.c_str());
				parse_request_data(get, sep + 1);
			} else {
				path = url;
			}

			state = HEADER;
			break;
		}
		case HEADER:
		{
			// no more headers
			if (line.empty()) {
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
				http_error(400, S("Malformed header."));
				return;
			}

			// determine which header we're dealing with
			if (!strncasecmp("Content-type", line.c_str(), c - line.c_str())) {
				// POST: content-type (must be application/x-www-form-urlencoded
				if (!strcmp("application/x-www-form-urlencoded", c + 2))
					posttype = URLENCODED;
				else {
					http_error(406, S("Content-type for POST must be application/x-www-form-urlencoded."));
					return;
				}
			} else if (!strncasecmp("Content-length", line.c_str(), c - line.c_str())) {
				// POST: content-length
				content_length = strtoul(c + 2, NULL, 10);

				// check max content length
				if (content_length > HTTP_POST_BODY_MAX) {
					http_error(413, S("Form data length overflow."));
				}
			} else if (!strncasecmp("Cookie", line.c_str(), c - line.c_str())) {
				// Cookie; look for session
				const char* sid_start;
				if ((sid_start = strstr(c + 2, "session=")) != NULL) {
					sid_start = strchr(sid_start, '=') + 1;
					const char* sid_end = strchr(sid_start, ';');
					std::string sid;
					if (sid_end == NULL)
						sid = std::string(sid_start);
					else
						sid = std::string(sid_start, sid_end - sid_start);

					// get the session (may be NULL if expired/invalid)
					session = HTTPManager.get_session(sid);
					if (session != NULL)
						session->update_timestamp();
				}
			} else {
				// ignore it
			}

			break;
		}
		case BODY:
		{
			// parse the post data
			parse_request_data(post, line.c_str());

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
HTTPHandler::parse_request_data (std::map<std::string,std::string>& map, const char* line) const
{
	// parse the data
	StringBuffer value;
	const char* begin;
	const char* end;
	const char* sep;

	begin = line;
	do {
		// find the end of the current pair
		end = strchr(begin, '&');
		if (end == NULL)
			end = line + strlen(line);

		// find the = separator
		sep = strchr(begin, '=');
		if (sep == NULL)
			sep = end;

		// decode the name 
		value.clear();
		for (const char* c = begin; c < sep; ++c) {
			if (*c == '+') {
				value << ' ';
			} else if (*c == '%') {
				int r;
				sscanf(c + 1, "%2X", &r);
				value << (char)tolower(r);
				c += 2;
			} else {
				value << (char)tolower(*c);
			}
		}

		// get reference to value
		std::string& vref = map[value.str()];

		// decode the value, if we have one
		if (sep != end) {
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
		}

		// store the data;
		// if we didn't parse a value above, the value by default
		// is the same as the name
		vref = value.str();

		// set begin to next token
		begin = end;
		if (*begin == '&')
			++begin;
	} while (*begin != 0);
}

void
HTTPHandler::execute()
{

	// DEBUG: Log request variables
	/*
	for (std::map<std::string,std::string>::iterator i = get.begin(); i != get.end(); ++i)
		Log::Info << "GET: " << i->first << "=" << i->second;
	for (std::map<std::string,std::string>::iterator i = post.begin(); i != post.end(); ++i)
		Log::Info << "POST: " << i->first << "=" << i->second;
	*/

	// handle built-in pages
	if (path == "/")
		page_index();
	else if (path == "/login")
		page_login();
	else if (path == "/logout")
		page_logout();
	else if (path == "/account")
		page_account();
	else {
		http_error(404, S("Page does not exist."));
		return;
	}

	// log access
	Log::Network << (reqtype == GET ? "GET" : "POST") << " " << url << " 200 " << (get_account() ? get_account()->get_id() : S("-")) << " " << Network::get_addr_name(addr);

	// done
	state = DONE;
}

void
HTTPHandler::page_index()
{
	*this << "HTTP/1.0 200 OK\nContent-type: text/html\n\n"
		<< StreamMacro(HTTPManager.get_template(S("header")), S("account"), get_account())
		<< StreamMacro(HTTPManager.get_template(S("index")), S("account"), get_account())
		<< StreamMacro(HTTPManager.get_template(S("footer")));
}

void
HTTPHandler::page_login()
{
	std::string msg;

	*this << "HTTP/1.0 200 OK\nContent-type: text/html\n";

	// attempt login?
	if (get_post(S("action")) == "Login") {
		Account* account = AccountManager.get(get_post(S("username")));
		if (account != NULL && account->check_passphrase(get_post(S("password")))) {
			session = HTTPManager.create_session(account);
			msg = S("Login successful!");
			*this << "Set-cookie: session=" << session->get_id() << "\n";
		} else {
			msg = S("Incorrect username or passphrase.");
		}
	}

	*this << "\n"
		<< StreamMacro(HTTPManager.get_template(S("header")), S("msg"), msg, S("account"), get_account())
		<< StreamMacro(HTTPManager.get_template(S("login")), S("account"), get_account())
		<< StreamMacro(HTTPManager.get_template(S("footer")));
}

void
HTTPHandler::page_logout()
{
	if (session != NULL)
		HTTPManager.destroy_session(session);

	*this << "HTTP/1.0 200 OK\nContent-type: text/html\nSet-cookie: session=\n\n"
		<< StreamMacro(HTTPManager.get_template(S("header")))
		<< StreamMacro(HTTPManager.get_template(S("logout")))
		<< StreamMacro(HTTPManager.get_template(S("footer")));
}

void
HTTPHandler::page_account()
{
	// must have an account for this to work
	if (get_account() == NULL) {
		http_error(403, S("You must login to access this page."));
		return;
	}

	std::string msg;
	if (get_post(S("action")) == "Save Changes") {
		std::string name = strip(post[S("acct_name")]);
		std::string email = strip(post[S("acct_email")]);
		if (name.empty() || email.empty()) {
			msg = S("You may not enter an empty name or email address.");
		} else {
			std::string pass1 = post[S("new_pass1")];
			std::string pass2 = post[S("new_pass2")];

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
		<< StreamMacro(HTTPManager.get_template(S("header")), S("account"), get_account(), S("msg"), msg)
		<< StreamMacro(HTTPManager.get_template(S("account")), S("account"), get_account())
		<< StreamMacro(HTTPManager.get_template(S("footer")));
}

void
HTTPHandler::http_error (int error, std::string msg)
{
	// lookup HTTP error msg code
	std::string http_msg;
	switch (error) {
		case 200: http_msg=S("OK"); break;
		case 400: http_msg=S("Bad Request"); break;
		case 403: http_msg=S("Access Denied"); break;
		case 404: http_msg=S("Not Found"); break;
		case 405: http_msg=S("Method Not Allowed"); break;
		case 406: http_msg=S("Not Acceptable"); break;
		case 408: http_msg=S("Request Timeout"); break;
		case 413: http_msg=S("Request Entity Too Large"); break;
		default: http_msg = S("Unknown"); break;
	}

	// display error page
	*this << "HTTP/1.0 " << error << " " << http_msg << "\nContent-type: text/html\n\n"
		<< StreamMacro(HTTPManager.get_template(S("header")), S("account"), get_account())
		<< StreamMacro(HTTPManager.get_template(S("error")), S("error"), tostr(error), S("http_msg"), http_msg, S("msg"), msg)
		<< StreamMacro(HTTPManager.get_template(S("footer")));

	// log error
	Log::Network << (reqtype == GET ? "GET" : reqtype == POST ? "POST" : "-") << " " << (url.empty() ? "-" : url) << " " << error << " " << (get_account() ? get_account()->get_id() : S("-")) << " " << Network::get_addr_name(addr) << " <" << msg << ">";

	// set error state
	state = ERROR;
}

std::string
HTTPHandler::get_request (std::string id) const
{
	std::map<std::string,std::string>::const_iterator i = get.find(id);
	if (i == get.end())
		return std::string();
	return i->second;
}

std::string
HTTPHandler::get_post (std::string id) const
{
	std::map<std::string,std::string>::const_iterator i = post.find(id);
	if (i == post.end())
		return std::string();
	return i->second;
}

int
HTTPHandler::macro_property (const StreamControl& stream, std::string method, const MacroList& argv) const
{
	if (method == "post" && argv.size() == 1) {
		stream.stream_put(get_post(argv[0].get_string()));
		return 0;
	} else {
		return -1;
	}
}

void
HTTPHandler::macro_default (const StreamControl& stream) const
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
	buf << "Source MUD";
	buf << account->get_id();
	buf << account->get_name();
	buf << rand();
	buf << account->get_email();
	buf << timestamp;
	char md5buf[MD5_BUFFER_SIZE];
	MD5::hash(buf.c_str(), md5buf);
	id = std::string(md5buf);
}

void
HTTPSession::update_timestamp ()
{
	timestamp = time(NULL);
}

bool
HTTPSession::check_timestamp ()
{
	return (time(NULL) - timestamp) < SettingsManager.get_http_timeout() * 60;
}

std::string
HTTPSession::get_var (std::string id) const
{
	std::map<std::string,std::string>::const_iterator i = vars.find(id);
	if (i == vars.end())
		return std::string();
	return i->second;
}

void
HTTPSession::set_var (std::string id, std::string val)
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
SHTTPManager::initialize (void)
{
	StringBuffer buf;

	// read templates
	StringList files = file::getFileList(SettingsManager.get_html_path(),
			S("tpl"));
	for (StringList::iterator i = files.begin(); i != files.end(); ++i) {
		FILE* in = fopen(i->c_str(), "rt");
		if (in == NULL) {
			Log::Error << "Failed to load HTML template " << *i << ": " << strerror(errno);
			return -1;
		}

		// read in buffer
		buf.clear();
		char line[512];
		while (fgets(line, sizeof(line), in) != NULL)
			buf << line;

		// clean up
		fclose(in);
		templates[base_name(i->c_str())] = buf.str();
	}

	// all good
	return 0;
}

void
SHTTPManager::shutdown (void)
{
	templates.clear();
}

std::string
SHTTPManager::get_template (std::string id)
{
	TemplateMap::iterator i = templates.find(id);
	return i != templates.end() ? i->second : std::string();
}

HTTPSession*
SHTTPManager::create_session (Account* account)
{
	assert(account != NULL);

	HTTPSession* session = new HTTPSession(account);
	sessions[session->get_id()] = session;

	return session;
}

void
SHTTPManager::destroy_session (HTTPSession* session)
{
	assert(session != NULL);

	SessionMap::iterator i = sessions.find(session->get_id());
	if (i != sessions.end())
		sessions.erase(i);
	session->clear();
	session = NULL;
}

HTTPSession*
SHTTPManager::get_session (std::string id)
{
	SessionMap::iterator i = sessions.find(id);
	if (i == sessions.end())
		return NULL;
	return i->second;
}

void
SHTTPManager::check_timeouts ()
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

const StreamControl&
operator << (const StreamControl& stream, const StreamHTTPEscape& esc)
{
	for (std::string::const_iterator i = esc.text.begin(); i != esc.text.end(); ++i) {
		if (*i == '<')
			stream.stream_put("&lt;");
		else if (*i == '>')
			stream.stream_put("&gt;");
		else if (*i == '&')
			stream.stream_put("&amp;");
		else if (*i == '"')
			stream.stream_put("&quot;");
		else
			stream.stream_put(&*i, 1);
	}
	return stream;
}
