<?php
	// AweMUD NG - Next Generation AwesomePlay MUD
	// Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
	// See the file COPYING for license details
	// http://www.awemud.net

	// This module makes it easy to issue commands to AweMUD over the control
	// socket interface.

	// AweMUD_Connect (path)
	//  Connects to the AweMUD socket at the given path
	function AweMUD_Connect($socket_path)
	{
		// Connect
		$socket = socket_create(AF_UNIX, SOCK_STREAM, 0);
		socket_connect($socket, $socket_path);
		return $socket;
	}

	// AweMUD_Command (socket, command, args)
	//  Issues the given command to the socket.  The args paramater is to be
	//  an array of arguments to pass in, which are automatically escaped.
	//  
	//  The return value is an array comprised of:
	//   (errcode, errmsg, response)
	//  errcode: a string representing the error code (OK means no //  error)
	//  errmsg: a string giving more information about the error code
	//  response: an array of lines of data returned by the command
	function AweMUD_Command($socket, $command, $args) {
		// format output line
		$line = str_replace('\n', '\\n', addslashes($command)); foreach($args as $arg)
		$line .= ' "' . addslashes($arg) . '"';

		// issue the command
		socket_write($socket, $line);

		// get response lines until we get an errcode line
		$data = array();
		while ($response = socket_read($socket, 1024, PHP_NORMAL_READ)) {
			// this is a errcode line, all done with command output
			if ($response[0] == '+') {
				list($errcode, $errmsg) = explode(substr($response, 1), ' ', 2);
				return array($errcode, $errmsg, $data);
			// this is a data line
			} else if ($response[0] == '-') {
				$data[]= substr($response, 1);
			} else {
				// FIXME: invalid line, show an error msg or something
			}
		}

		// early failure, possibly we lost connection
		return array('NETWORK', 'Connection lost', array());
	}

	// AweMUD_Close (socket)
	//  Closes the connection to the control socket.
	function AweMUD_Control($socket)
	{
		socket_close($socket);
	}
?>
