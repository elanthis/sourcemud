-- Source MUD
-- Copyright (C) 2008  Sean Middleditch
-- See the file COPYING for license details
-- http://www.sourcemud.org
--
-- HTTP REQUEST HANDLER
-- This registers a function as the http_request hook.  This hook is
-- called for every single HTTP request made on our HTTP handler.
--
-- If the incoming path is something we care to handle, then we process
-- the request (which includes printing headers as well as the content
-- body) and then return the HTTP response code we wish to log with the
-- request.  This is 200 is a regular page is served, generally.
--
-- We also have the option of asking for a "passthru" to the built-in
-- file handler, which looks for files in the ./data/html/ directory.
-- For this, we return 0.
--
-- If an error is raised, then the HTTP subsystem will log it as error
-- code 500 (internal server error).
--
-- Note that to print date, we use the global print() function, which
-- is set to direct out to the client.  The HTTP sybsystem does no
-- validation of our output, so if we send broken HTTP, the server will
-- not fix it for us.
--
-- The req parameter passed to http_request is a table with the following
-- elements:
--   GET: a table containing the URL query parameters
--   POST: a table containing POST parameters (for URL-encoded POST requests)
--   HEADER: a table containing all HTTP headers, with lower-cased keys
--   COOKIE: a table containing all cookies, with lower-cased keys
--   REQUEST: a table containing an a combination of all of the above
--   url: the full URL used in the request
--   method: the HTTP method of the request
--   path: the URL, minus any query or link target components, in the request

(function() -- begin private namespace

function http_request(req)
	-- our test page
	if req.path == '/test' then
		print "HTTP/1.1 200 OK\r\n"
		print "Content-Type: text/html\r\n\r\n"
		print "<h1>Test!</h1>\n"
		return 200
	end

	-- we didn't handle it; pass-thru
	return 0
end

mud.setHook('http_request', http_request)

end)() -- end private namspace
