/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef __SOURCEMUD_MAIL_H__
#define __SOURCEMUD_MAIL_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SENDMAIL

#include "common/gcvector.h"
#include "common/string.h"
#include "common/strbuf.h"

class MailMessage : public GC {
	private:
	String to;
	String subject;
	StringBuffer body;
	struct Header {
		String name;
		String value;
	};
	GCType::vector<Header> headers;

	public:
	MailMessage (String s_to, String s_subj, String s_body) :
		to(s_to), subject(s_subj), body(), headers() { body << s_body; }
	MailMessage (String s_to, String s_subj) :
		to(s_to), subject(s_subj), body(), headers() {}

	void append (String data);

	void header (String name, String value);

	int send (void) const;
};

#endif // HAVE_SENDMAIL

#endif
