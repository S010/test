#include <sys/types.h>
#include <sys/socket.h>
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
pump(struct pollfd *pfds, size_t n_pfds)
{
	size_t			 i, j;
	ssize_t			 n_read, n_write;
	char			 buf[16 * 1024];
	size_t			 bufsize = sizeof buf;

	for (i = 0; i < n_pfds; ++i) {
		if (pfds[i].revents & POLLIN) {
			while ((n_read = read(pfds[i].fd, buf, bufsize)) > 0) {
				printf("pumping %d bytes of data...\n", (int) n_read);
				for (j = 0; j < n_pfds; ++j) {
					if (j == i)
						continue;
					do n_write = write(pfds[j].fd, buf, n_read);
					while (n_write == -1 && errno == EAGAIN);
				}
			}
			if (n_read < 0)
				warn("read");
		}
	}
}

static void
del_pfd(struct pollfd **pfds, size_t *n_pfds, size_t idx)
{
	memmove(*pfds + idx, *pfds + idx + 1, sizeof(**pfds) * (*n_pfds - idx - 1));
	*pfds = xrealloc(*pfds, sizeof(**pfds) * --(*n_pfds));
}

static void
close_erronous(struct pollfd **pfds, size_t *n_pfds)
{
	size_t	 i;

	for (i = 1; i < *n_pfds; /* empty */) {
		if ((*pfds)[i].revents & (POLLERR | POLLHUP)) {
			printf("shutting down a socket...\n");
			shutdown((*pfds)[i].fd, SHUT_RDWR);
			del_pfd(pfds, n_pfds, i);
		} else {
			++i;
		}
	}
}


static void
tcp_hub(int port)
{
	int			 s;
	struct sockaddr_in	 sa;
	struct sockaddr_storage	 sa_stor;
	socklen_t		 sa_stor_size;
	int			 conn;
	struct pollfd		*pfds = NULL;
	size_t			 n_pfds = 0;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
		err(1, "socket");
	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_port = htons((short) port);
	sa.sin_addr.s_addr = INADDR_ANY;
	if (bind(s, (struct sockaddr *) &sa, sizeof sa) != 0)
		err(1, "bind");
	printf("listening...\n");
	if (listen(s, INT_MAX) != 0)
		err(1, "listen");

	pfds = xrealloc(pfds, sizeof(*pfds) * ++n_pfds);
	pfds[0].fd = s;
	pfds[0].events = POLLIN;
	for ( ;; ) {
		if (poll(pfds, n_pfds, INFTIM) == -1)
			err(1, "poll");
		if (pfds[0].revents & POLLIN) { /* received a new connection */
			conn = accept(s, (struct sockaddr *) &sa_stor, &sa_stor_size);
			if (conn == -1)
				err(1, "accept");
			printf("accepted a new connection...\n");
			if (fcntl(conn, F_SETFL, O_NONBLOCK) == -1)
				err(1, "fcntl");
			pfds = xrealloc(pfds, sizeof(*pfds) * ++n_pfds);
			pfds[n_pfds - 1].fd = conn;
			pfds[n_pfds - 1].events = POLLIN;
		}
		pump(pfds + 1, n_pfds - 1);
		close_erronous(&pfds, &n_pfds);
	}
}

int
main(int argc, char **argv)
{
	int	 port;

	if (argc < 2)
		return 1;

	port = atoi(argv[1]);
	tcp_hub(port);

	return 0;
}

