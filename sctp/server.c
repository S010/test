#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <err.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	(void)argc, (void)argv;

	puts("socket");
	int s = socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (s == -1) {
		err(1, "socket");
	}

	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(1234);
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	puts("sctp_bindx");
	if (sctp_bindx(s, (struct sockaddr *)&sa, 1, SCTP_BINDX_ADD_ADDR) == -1) {
		err(1, "sctp_bindx");
	}

	if (listen(s, 1) == -1) {
		err(1, "listen");
	}

	uint8_t buf[8192];
	for (;;) {
		puts("sctp_recvmsg");
		int n = sctp_recvmsg(s, buf, sizeof(buf)-1, NULL, NULL, NULL, NULL);
		if (n == -1) {
			err(1, "sctp_recvmsg");
		}
		buf[n] = '\0';
		printf("received msg: %s\n", buf);
	}

	return 0;
}
