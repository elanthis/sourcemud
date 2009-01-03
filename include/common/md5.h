/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MD5_H
#define SOURCEMUD_MD5_H

#define MD5_BUFFER_SIZE 50

namespace MD5
{
// given a key, it creats a salt and encrypts in MD5,
// storing the result in buffer, which must be
// MD5_BUFFER_SIZE or larger (including NUL byte location)
void encrypt(const char *key, char *buffer);

// computes an MD5 hash of the source string,
// storing the result in buffer, which must be
// MD5_BUFFER_SIZE or larger (including NUL byte location)
void hash(const char* source, char* buffer);
std::string hash(const std::string&);

// given an already encrypted password (including the salt)
// in base, and an unencrypted password in pass, it compares
// the two and returns true if equal, and false otherwise
int compare(const char *base, const char *pass);
}

#endif
