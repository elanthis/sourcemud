#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define DEFAULT_SOCK "./awemud-ctrl.sock"

const char* self = "awemud-ctrl";

void
help (void)
{
	fprintf(stderr, "AweMUD Control\n");
	fprintf(stderr, "Usage: %s <file>\n", self);
	fprintf(stderr, "Commands:\n");
	fprintf(stderr, "  info      - display server information\n");
}

void
dowrite (int sock, const char* text, int len)
{
	int res;

	while (len > 0) {
		res = write(sock, text, len);
		if (res <= 0) {
			fprintf(stderr, "Fatal write error: %s\n", strerror(errno));
			exit(1);
		}
		text -= res;
		len -= res;
	}
}

int
main (int argc, char** argv)
{
	char buffer[1024];
	const char* path = DEFAULT_SOCK;
	struct sockaddr_un addr;
	size_t addr_len;
	fd_set watch;
	fd_set ready;
	int sock;
	int blen;

	// too many arguments?
	if (argc > 2) {
		help();
		exit(1);
	}

	// asked for help?
	if (argc == 2 && !strcmp(argv[1], "--help")) {
		help();
		exit(0);
	}

	// get path
	if (argc == 2)
		path = argv[1];

	// open socket
	if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
		exit(1);
	}

	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);
	addr_len = sizeof(addr.sun_family) + strlen(addr.sun_path);

	if (connect(sock, (struct sockaddr*)&addr, addr_len)) {
		fprintf(stderr, "Failed to connect socket: %s\n", strerror(errno));
		exit(1);
	}

	// setup for loop
	FD_ZERO(&watch);
	FD_SET(0, &watch);
	FD_SET(sock, &watch);

	// read loop
	while (1) {
		ready = watch;
		if (select(sock + 1, &ready, NULL, NULL, NULL) < 0) {
			fprintf(stderr, "Failed to poll: %s\n", strerror(errno));
			exit(1);
		}

		// read console input
		if (FD_ISSET(0, &ready)) {
			// read
			blen = read(0, buffer, sizeof(buffer));
			if (blen < 0 && errno != EAGAIN && errno != EINTR) {
				fprintf(stderr, "Fatal console error: %s\n", strerror(errno));
				exit(1);
			}
			if (blen == 0) {
				exit(0);
			}

			// send
			dowrite(sock, buffer, blen);
		}

		// read sock input
		if (FD_ISSET(sock, &ready)) {
			// read
			blen = read(sock, buffer, sizeof(buffer));
			if (blen < 0 && errno != EAGAIN && errno != EINTR) {
				fprintf(stderr, "Fatal socket error: %s\n", strerror(errno));
				exit(1);
			}
			if (blen == 0) {
				exit(0);
			}

			// send
			printf("%.*s", blen, buffer);
		}
	}

	return 0;
}
