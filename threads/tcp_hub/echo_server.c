#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <xmem.h>

static void
echo(int conn)
{
	struct pollfd	 pfds[1];
	size_t		 n_pfds = sizeof(*pfds) / sizeof(pfds[0]);
	char		 buf[1024 * 16];
	const size_t	 bufsize = sizeof buf;
	ssize_t		 n_read;

	pfds->fd = conn;
	pfds->events = POLLIN;

	while (poll(pfds, n_pfds, -1)) {
		if (pfds->revents & (POLLHUP | POLLERR)) {
			shutdown(pfds->fd, SHUT_RDWR);
			return;
		} else if (pfds->revents & POLLIN) {
			while ((n_read = read(pfds->fd, buf, bufsize)) > 0)
				write(pfds->fd, buf, n_read);
		}
	}
}

static void
echo_server(int port)
{
	int			 s;
	struct sockaddr_in	 sa;
	int			 conn;
	struct sockaddr_storage	 sa_stor;
	socklen_t		 sa_stor_size;
	pid_t			 pid;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
		err(1, "socket");
	sa.sin_family = AF_INET;
	sa.sin_port = htons((short) port);
	sa.sin_addr.s_addr = INADDR_ANY;
	if (bind(s, (struct sockaddr *) &sa, sizeof sa) != 0)
		err(1, "bind");
	if (listen(s, INT_MAX) != 0)
		err(1, "listen");

	for ( ;; ) {
		conn = accept(s, (struct sockaddr *) &sa_stor, &sa_stor_size);
		if (conn == -1)
			err(1, "conn");
		if (fcntl(conn, F_SETFL, O_NONBLOCK) == -1)
			err(1, "fcntl");
		pid = fork();
		if (pid < 0) {
			err(1, "fork");
		} else if (pid == 0) {
			close(s);
			echo(conn);
			return;
		} else {
			close(conn);
		}
	}
}

int
main(int argc, char **argv)
{
	int	 port;

	if (argc < 2)
		return 1;

	port = atoi(argv[1]);
	echo_server(port);

	return 0;
}

