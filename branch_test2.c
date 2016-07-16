#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <err.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
open_ctl_socket1(const char *path)
{
	struct sockaddr_un addr;
	int s;

	memset(&addr, 0, sizeof addr);
	addr.sun_family = AF_LOCAL;
	strlcpy(addr.sun_path, path, sizeof(addr.sun_path));

	unlink(path);

	s = socket(PF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (s == -1)
		err(1, "socket");

	if (bind(s, (void *)&addr, sizeof addr) == -1)
		err(1, "bind");

	if (listen(s, 1) == -1)
		err(1, "listen");

	return s;
}

int
open_ctl_socket2(const char *path)
{
	struct sockaddr_un addr;
	int s;

	memset(&addr, 0, sizeof addr);
	addr.sun_family = AF_LOCAL;
	strlcpy(addr.sun_path, path, sizeof(addr.sun_path));

	unlink(path);

	if ((s = socket(PF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0)) != -1
	    && bind(s, (void *)&addr, sizeof addr) != -1
	    && listen(s, 1) != -1) {
		return s;
	}

	err(1, "failed to open socket");
	return -1;
}

void
read_msgs(int s)
{
	struct pollfd pfd;
	int n;
	int c;
	char buf[64];

	for (;;) {
		pfd.fd = s;
		pfd.events = POLLIN;

		n = poll(&pfd, 1, INFTIM);
		if (n == -1)
			err(1, "poll");
		else if (n == 0)
			continue;

		c = accept(s, NULL, NULL);
		if (c == -1)
			err(1, "accept");

		n = read(c, buf, sizeof(buf) - 1);
		if (n > 0) {
			buf[sizeof(buf) - 1] = '\0';
			printf("msg: %s\n", buf);
		}
		close(c);
	}
}

int
main(int argc, char **argv)
{
	int s;

	int (*f)(const char *);

	if (argc < 2)
		return 1;

	if (!strcmp(argv[1], "1"))
		f = open_ctl_socket1;
	else
		f = open_ctl_socket2;

	return 0;
}
