#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <poll.h>

#define DEFAULT_PORT	1234
#define POLL_TIMEOUT	-1

void usage(void);
void server(unsigned short);

struct pfds {
	struct pollfd	*fds;
	int		 n;
};

void
pfds_init(struct pfds *p)
{
	p->fds = NULL;
	p->n = 0;
}

int
pfds_add(struct pfds *p, int fd, short events)
{
	struct pollfd *q;
	if (p->n == INT_MAX) {
		warnx("pfds_add: maximum number of pollfds reached");
		return -1;
	}
	q = realloc(p->fds, sizeof(*p->fds) * (p->n + 1));
	if (q == NULL) {
		warn("pfds_add: realloc");
		return -1;
	}
	p->fds = q;
	++p->n;
	return 0;
}

int
pfds_del(struct pfds *p, int fd)
{
	struct pollfd	*q;
	struct pollfd	*last = p->fds + n - 1;
	struct pollfd	 tmp;

	for (struct pollfd *q = p->fds; q <= last; ++q) {
		if (q->fd != fd)
			continue;
		if (q != last) {
			tmp = *last;
			*last = *q;
			*q = tmp;
		}
		q = realloc(p->fds, sizeof(*p->fds) * (p->n - 1));
		if (q != NULL) {
			warn("pfds_del: realloc");
			p->fds = q;
		}
		--p->n;
		return 0;
	}
	return -1;
}

int
main(int argc, char **argv)
{
	int		 ch;
	unsigned short	 port = DEFAULT_PORT;
	extern char	*optarg;

	while ((ch = getopt(argc, argv, "hp:")) != -1) {
		switch (ch) {
		case 'h':
			usage();
			return 0;
		case 'p':
			port = atoi(optarg);
			break;
		default:
			usage();
			return 1;
		}
	}

	server(port);

	return 0;
}

void
usage(void)
{
	fprintf(stderr, "usage: server [-p <port>]\n"
	    "Default port is 1234.\n");
}

void
setnonblock(int fd)
{
	if (fcntl(fd, F_SETFD, O_NONBLOCK) == -1)
		err(1, "setnonblock: fcntl");
}

void
server(unsigned short port)
{
	int			 s;
	int			 conn;
	struct sockaddr_in	 sa;
	ssize_t			 nread;
	struct			 pfds;
        int			 npoll;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
		err(1, "socket");
        setnonblock(s);
	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (struct sockaddr *) &sa, sizeof sa) == -1)
		err(1, "bind");
	if (listen(s, 0) == -1)
		err(1, "listen");

	pfds_init(&pfds);
	pfds_add(&pfds, s, POLLIN | POLLOUT);

	while ((npoll = poll(pfds.fds, pfds.n, POLL_TIMEOUT)) != -1) {
		/* // ???
		conn = accept(s, NULL, NULL);
		if (conn == -1) {
			warn("accept");
			continue;
		}

		while ((nread = read(conn, buf, bufsize)) > 0)
			write(conn, buf, nread);
		close(conn);
		*/
	}
	if (npoll < 0)
		err(1, "server: poll");
}
