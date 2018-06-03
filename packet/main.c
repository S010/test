#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char **argv)
{
	int s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (s == -1) {
		err(1, "socket");
	}

	uint8_t buf[1500];
	struct sockaddr_ll *sall = (struct sockaddr_ll *)buf;
	int n;
	while ((n = recv(s, buf, sizeof buf, 0)) > 0) {
		printf("got packet (%d bytes)\n", n);
	}
	if (n < 0) {
		err(1, "read");
	}

	return 0;
}

