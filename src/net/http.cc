/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/error.h"
#include "common/streams.h"
#include "common/log.h"
#include "common/md5.h"
#include "common/file.h"
#include "common/rand.h"
#include "common/streamtime.h"
#include "common/string.h"
#include "mud/server.h"
#include "mud/settings.h"
#include "net/http.h"
#include "net/manager.h"
#include "net/util.h"
#include "lua/core.h"
#include "lua/print.h"
#include "lua/exec.h"

#define HTTP_REQUEST_TIMEOUT 30 // 30 seconds
#define HTTP_HEADER_LINE_MAX 2048 // arbitrary max line length
#define HTTP_POST_BODY_MAX (16*1024) // 16K

SHTTPManager HTTPManager;

namespace Log
{
	LogWrapper HTTP(LOG_HTTP);
}

HTTPHandler::HTTPHandler(int s_sock, const NetAddr& s_netaddr) :  SocketConnection(s_sock)
{
	addr = s_netaddr;
	state = REQ;
	timeout = time(NULL);
	account = NULL;
}

// disconnect
void
HTTPHandler::disconnect()
{
	// reduce count
	MNetwork.connections.remove(addr);

	// close socket
	sock_disconnect();
}

/* output a data of text -
 * deal with formatting new-lines and such, and also
 * escaping/removing/translating Source MUD commands
 */
void
HTTPHandler::stream_put(const char *text, size_t len)
{
	sock_buffer(text, len);
}

// process input
void
HTTPHandler::sock_input(char* buffer, size_t size)
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
					http_error(413);
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
HTTPHandler::sock_flush()
{
	// handle timeout
	if (timeout != 0 && (time(NULL) - timeout) >= HTTP_REQUEST_TIMEOUT) {
		http_error(408);
		timeout = 0;
	}

	// disconnect if we are all done
	if (state == DONE || state == ERROR)
		disconnect();
}

void
HTTPHandler::sock_hangup()
{
	disconnect();
}

void HTTPHandler::process()
{
	switch (state) {
	case REQ: {
		request = line.str();

		// empty request?  ignore
		if (request.empty())
			return;

		// parse
		std::vector<std::string> parts = explode(request, ' ');

		// check size
		if (parts.size() != 3) {
			http_error(400);
			return;
		}

		// http method
		method = parts[0];
		if (method != "GET" && method != "POST") {
			http_error(405);
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
		File::normalize(path);

		state = HEADER;
		break;
	}
	case HEADER: {
		// no more headers
		if (line.empty()) {
			// determine content length of body, if any
			content_length = tolong(getHeader("content-length"));
			if (content_length > HTTP_POST_BODY_MAX) {
				http_error(413);
				// if we have no content length, go straight to processing
			} else if (content_length == 0) {
				execute();
				state = DONE;
				// we must continue on with processing body
			} else {
				state = BODY;
			}
			break;
		}

		// parse the header
		const char* c = strchr(line.c_str(), ':');
		// require ': ' after header name
		if (c == NULL || *(c + 1) != ' ') {
			http_error(400);
			return;
		}

		// store it away, lower-case the name
		header[strlower(std::string(line.c_str(), c - line.c_str()))] = c + 2;

		/*
					// determine which header we're dealing with
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

							// split session into hash, salt, and account ID
							std::vector<std::string> parts = explode(sid, ':');
							if (parts.size() == 3) {
								std::string hash = parts[0];
								std::string salt = parts[1];
								std::string id = parts[2];

								// re-hash and compare
								std::ostringstream buf;
								buf << HTTPManager.get_session_key() << ':' << salt << ':' << id;
								if (hash == MD5::hash(buf.str())) {
									// lookup the account, we're successful
									account = AccountManager.get(id);
								}
							}
						}
		*/

		break;
	}
	case BODY: {
		// parse the post data, we we can
		if (getHeader("content-type") == "application/x-www-form-urlencoded")
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

void HTTPHandler::parse_request_data(std::map<std::string, std::string>& map, const char* line) const
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

void HTTPHandler::execute()
{
	// set up to run the Lua http_request hook handler
	Lua::ExecHook exec("http_request");

	// build the 'req' table parameter
	exec.table();
	exec.setTable("url", url);
	exec.setTable("path", path);
	exec.setTable("method", method);

	// set ourself as the print handler
	exec.setPrint(this);

	// run; fail with HTTP 500 on error
	if (!exec.run()) {
		http_error(500);
		return;
	}

	// if the return value is non-0, log the response code
	int code = exec.getInteger();
	if (code != 0) {
		log(code);
		return;
	}

	// the response code was zero -- try to serve a file the
	// old-fashioned way.  cleanup Lua first, then turn the
	// url request into a full path
	exec.cleanup();

	std::string file = MSettings.get_html_path() + path;

	// check to see if our file exists, and serve it if it does
	if (File::isfile(file)) {
		serve(file);
		return;
	}

	// try it with a directory index applied (FIXME: kinda hacky)
	file += "/index.html";
	if (File::isfile(file)) {
		serve(file);
		return;
	}

	// not found
	http_error(404);
}

void HTTPHandler::serve(const std::string& full_path)
{
	// get mime type
	const std::string& mime = File::getMimeType(full_path);

	// stat the file
	struct stat st;
	if (stat(full_path.c_str(), &st) != 0) {
		http_error(404);
		return;
	}

	// calculate mtime, size, and etag
	std::string mtime = Time::format(Time::RFC_822_FORMAT, st.st_mtime);
	std::string etag = '"' + MD5::hash(full_path + mtime) + '"';
	size_t size = st.st_size;

	// see if the item should be cached
	if (getHeader("if-modified-since") == mtime
	        && getHeader("if-none-match") == etag) {
		*this <<
		"HTTP/1.1 304 Not Modified\r\n"
		"ETag: " << etag << "\r\n\r\n";
		log(304);
		return;
	}

	// simple headers
	*this <<
	"HTTP/1.0 200 OK\r\n"
	"Content-Type: " << mime << "\r\n"
	"Content-Length: " << size << "\r\n"
	"Last-Modified: " << mtime << "\r\n"
	"ETag: " << etag << "\r\n\r\n";

	// file content
	std::ifstream ifs;
	ifs.open(full_path.c_str(), std::ios::binary);
	char buf[1024];
	size_t rs = 0;
	while (ifs) {
		rs = ifs.readsome(buf, sizeof(buf));
		if (rs == 0)
			break;
		this->stream_put(buf, rs);
	}
	ifs.close();

	// successful
	log(200);
}

void HTTPHandler::log(int error)
{
	// get headers we log
	const std::string& user_agent = getHeader("user-agent");
	const std::string& referer = getHeader("referer");
	Log::HTTP
	<< addr.getString(false) << ' '
	<< "- " // RFC 1413 identify -- apache log compatibility place-holder
	<< (get_account() ? get_account()->getId().c_str() : "-") << ' '
	<< '[' << StreamTime("%d/%b/%Y:%H:%M:%S %z") << "] "
	<< '"' << request << "\" "
	<< error << ' '
	<< get_out_bytes() << ' '
	<< '"' << (referer.empty() ? "-" : referer.c_str()) << "\" "
	<< '"' << (user_agent.empty() ? "-" : user_agent.c_str()) << '"';
}

void
HTTPHandler::http_error(int error)
{
	// lookup HTTP error msg code
	std::string http_msg;
	switch (error) {
	case 200:
		http_msg = "OK";
		break;
	case 400:
		http_msg = "Bad Request";
		break;
	case 403:
		http_msg = "Access Denied";
		break;
	case 404:
		http_msg = "Not Found";
		break;
	case 405:
		http_msg = "Method Not Allowed";
		break;
	case 406:
		http_msg = "Not Acceptable";
		break;
	case 408:
		http_msg = "Request Timeout";
		break;
	case 413:
		http_msg = "Request Entity Too Large";
		break;
	default:
		http_msg = "Unknown";
		break;
	}

	// display error page
	*this <<
	"HTTP/1.1 " << error << ' ' << http_msg << "\r\n"
	"Content-Type: text/html\r\n\r\n"
	"<html><head><title>Error</title></head>"
	"<body><h1>" << error << " Error</h1>"
	"<p>" << http_msg << "</p></body></html>";

	// log error
	log(error);

	// set error state
	state = ERROR;
}

const std::string& HTTPHandler::getHeader(const std::string& name) const
{
	std::map<std::string, std::string>::const_iterator i = header.find(name);
	if (i != header.end())
		return i->second;
	static const std::string empty;
	return empty;
}

const std::string& HTTPHandler::getCookie(const std::string& name) const
{
	std::map<std::string, std::string>::const_iterator i = cookie.find(name);
	if (i != cookie.end())
		return i->second;
	static const std::string empty;
	return empty;
}

const std::string& HTTPHandler::getGET(const std::string& name) const
{
	std::map<std::string, std::string>::const_iterator i = get.find(name);
	if (i != get.end())
		return i->second;
	static const std::string empty;
	return empty;
}

const std::string& HTTPHandler::getPOST(const std::string& name) const
{
	std::map<std::string, std::string>::const_iterator i = post.find(name);
	if (i != post.end())
		return i->second;
	static const std::string empty;
	return empty;
}

const std::string& HTTPHandler::getRequest(const std::string& name) const
{
	// searches POST. GET. then cookie
	std::map<std::string, std::string>::const_iterator i = post.find(name);
	if (i != post.end())
		return i->second;
	i = get.find(name);
	if (i != get.end())
		return i->second;
	i = cookie.find(name);
	if (i != cookie.end())
		return i->second;
	static const std::string empty;
	return empty;
}

int
SHTTPManager::initialize()
{
	StringBuffer buf;

	std::ifstream ifs(MSettings.get_skey_path().c_str());
	if (!ifs) {
		Log::Error << "Failed to open session key file " << MSettings.get_skey_path();
		return -1;
	}
	ifs >> session_key;
	ifs.close();

	// all good
	return 0;
}

void
SHTTPManager::shutdown()
{
}
