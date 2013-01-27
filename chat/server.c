#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#define DEFAULT_PORT	1234

void usage(void);
void server(unsigned short);

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
server(unsigned short port)
{
	int			 s;
	int			 conn;
	struct sockaddr_in	 sa;
	char			*buf;
	const size_t		 bufsize = 8192;
	size_t			 nread;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
		err(1, "socket");
	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (struct sockaddr *) &sa, sizeof sa) == -1)
		err(1, "bind");
	if (listen(s, 0) == -1)
		err(1, "listen");

	buf = malloc(bufsize);
	for ( ;; ) {
		conn = accept(s, NULL, NULL);
		if (conn == -1) {
			warn("accept");
			continue;
		}

		while ((nread = read(conn, buf, bufsize)) > 0)
			write(conn, buf, nread);
		close(conn);
	}
}
