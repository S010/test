#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <err.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
	(void)argc, (void)argv;

	puts("socket");
	int s = socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (s == -1) {
		err(1, "socket");
	}

	struct sockaddr_in sa;
	socklen_t sa_len = sizeof(sa);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(1234);
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	char buf[8192];
	size_t len;
	while (fgets(buf, sizeof(buf), stdin) != NULL) {
		puts("sctp_send");
		len = strlen(buf);
		if (len > 0 && buf[len-1] == '\n') {
			buf[len-1] = '\0';
		}
		if (sctp_sendmsg(s, buf, len, (struct sockaddr *)&sa, sa_len, 0, 0, 0, 1000, 0) == -1) {
			err(1, "sctp_sendmsg");
		}
	}

	return 0;
}
