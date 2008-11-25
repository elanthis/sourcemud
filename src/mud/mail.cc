/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifdef HAVE_SENDMAIL

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#include "common/mail.h"
#include "common/log.h"
#include "mud/settings.h"

#include "config.h"

void
MailMessage::append (String data)
{
	body << data;
}

void
MailMessage::header (String name, String value)
{
	Header h;
	h.name = name;
	h.value = value;
	headers.push_back(h);
}

int
MailMessage::send (void) const
{
	// sanity
	if (!to || !subject) {
		Log::Error << "Attempt to send a message without both a recipient and a subject.";
		return -1;
	}

	// configuration
	String sendmail = SettingsManager.get_sendmail_bin();
	if (!sendmail) {
		Log::Error << "No sendmail binary configured";
		return -1;
	}	 

	// pipes
	int files[2];
	if (pipe(files)) {
		Log::Error << "pipe() failed: " << strerror(errno);
		return -1;
	}

	// fork
	pid_t pid;
	if ((pid = fork()) == 0) {
		// file handles - erg
		close(files[1]);
		close (0); // stdin
		close (1); // stdout
		close (2); // stderr
		dup2(files[0], 0);
		// exec sendmail
		if (execl(sendmail.c_str(), sendmail.c_str(), "-oi", "-t", "-FSource MUD", NULL))
			exit(1);
	}
	close(files[0]);

	// failure on fork
	if (pid < 0) {
		Log::Error << "fork() failed: " << strerror(errno);
		return -1;
	}

	// write out data
	FILE* fout = fdopen(files[1], "w");
	if (fout == NULL) {
		Log::Error << "fdopen() failed: " << strerror(errno);

		// wait for child
		do {
			waitpid(pid, NULL, WNOHANG);
		} while (errno == EINTR);

		return -1;
	}
	// print
	fprintf(fout, "Subject: %s\n", subject.c_str());
	fprintf(fout, "To: %s\n", to.c_str());
	fprintf(fout, "X-Source MUD: YES\n");
	for (std::vector<Header>::const_iterator i = headers.begin(); i != headers.end(); ++i)
		fprintf (fout, "%s: %s\n", i->name.c_str(), i->value.c_str());
	fprintf(fout, "\n%s\n", body.c_str());
	fclose(fout);

	// wait for child
	do {
		waitpid(pid, NULL, WNOHANG);
	} while (errno == EINTR);

	return 0;
}

#endif // HAVE_SENDMAIL
