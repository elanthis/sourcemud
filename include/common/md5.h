/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_MD5_H
#define AWEMUD_MD5_H

#define MD5_BUFFER_SIZE 50

namespace MD5 {
	void encrypt (const char *key, char *buffer);
	// given a key, it creats a salt and encrypts in MD5,
	// storing the result in buffer, which must be
	// MD5_BUFFER_SIZE or larger (including NUL byte location)

	int compare (const char *base, const char *pass);
	// given an already encrypted password (including the salt)
	// in base, and an unencrypted password in pass, it compares
	// the two and returns true if equal, and false otherwise
}

#endif
