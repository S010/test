#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <err.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

//#include <xmem.h>

struct client {
	struct client	*next;
	int		 s;
};


void
tcp_hub(int port)
{
	int			 s;
	struct sockaddr_in	 sa;
	struct sockaddr_storage	 sa_stor;
	socklen_t		 sa_stor_size;
	int			 conn;
	int			*conns = NULL;
	size_t			 n_conns = 0;
	struct client		*clients = NULL;
	struct client		*client;


	struct pollfd		*pfds = NULL, *pfd;
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

	pfds = xrealloc(sizeof(*pfds) * ++n_pfds);
	pfds[0].fd = s;
	pfds[0].events = POLLIN;
	for ( ;; ) {
		if (poll(&pfds, n_pfds, INFTIM) == -1)
			err(1, "poll");
		if (pfds->revents & POLLIN) {
			conn = accept(s, (struct sockaddr *) &sa_stor, &sa_stor_size);
			if (conn == -1)
				err(1, "accept");
		}
		for (pfd = pfds + 1; pfd < (pfds + n_pfds); ++pfd) {
			// todo
		}

		printf("accepting a connection...\n");
		/*
		// todo
		client = xcalloc(1, sizeof *client);
		client->next = clients;
		client->s = conn;
		clients = client;
		conns = xrealloc(sizeof(*conns) * ++n_conns);
		conns[n_conns - 1] = conn;
		*/
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

