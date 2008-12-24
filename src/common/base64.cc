// base on code from http://gd.tuwien.ac.at/infosys/mail/vm/base64-decode.c
// cleaned up and modified for SourceMUD's needs
// this code is in the public domain

#include "common.h"
#include "common/base64.h"

namespace {
	const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
}

std::string Base64::decode(const std::string& str) {
	std::ostringstream buf;
	uint32_t bits = 0, count = 0;

	for (std::string::const_iterator i = str.begin(), e = str.end();
			i != e; ++i) {
		// we're at the padding, end now
		if (*i == '=')
			break;

		// find position in alphabet; skip if not found
		size_t pos = alphabet.find(*i);
		if (pos == std::string::npos)
			continue;

		// build up bits
		bits += pos;

		// every 4 characters results in 3 full bytes of output
		count++;
		if (count == 4) {
			// print out the three bytes of data as characters
			buf << (char)(bits >> 16);
			buf << (char)((bits >> 8) & 0xff);
			buf << (char)(bits & 0xff);
			bits = count = 0;
		} else {
			// shift our bits 6 places, to make room for the next
			// 6-bit base64 character
			bits <<= 6;
		}
	}

	// handle any hanging bits
	// NOTE: we do no checking on incomplete padding; garbage in, garbage out
	if (count == 3) { // one = would have been received
		buf << (char)(bits >> 16);
		buf << (char)((bits >> 8) & 0xff);
	} else if (count == 2) { // two = would have been received
		buf << (char)(bits >> 10);
	}

	return buf.str();
}
