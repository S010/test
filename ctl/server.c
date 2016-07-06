#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <string.h>

static void
handle_msg(int s)
{
	int c;

	c = accept(s, NULL, NULL);
	if (c < 0) {
		warn("accept");
		return;
	}

	char buf[8] = { 0 };

	if (read(c, buf, sizeof(buf) - 1) > 0) {
		printf("msg: ``%s''\n", buf);
	}

	close(c);
}

static void
server(void)
{
	int s;
	const char path[] = "ctl";
	struct pollfd pfd;
	int n;

	s = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (s == -1)
		err(1, "socket");

	(void)unlink(path);

	struct sockaddr_un a = { 0 };
	a.sun_family = AF_LOCAL;
	strlcpy(a.sun_path, path, sizeof(a.sun_path));

	if (bind(s, (void *)&a, sizeof a))
		goto err;

	if (listen(s, 0))
		goto err;


	for (;;) {
		pfd.fd = s;
		pfd.events = POLLIN;

		n = poll(&pfd, 1, -1);

		if (n < 0)
			err(1, "poll");
		else if (n == 0)
			continue;

		handle_msg(s);
	}

err:
	(void)close(s);
}

int
main(int argc, char **argv)
{
	server();
	return 0;
}
