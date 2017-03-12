/*
 * Copyright (c) 2017 Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define _POSIX_C_SOURCE 201112L
#define _DEFAULT_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <endian.h>

#include "protocol.h"

/*
 * A resizable data buffer.
 */
struct buffer {
	uint8_t *ptr;
	size_t len;
	size_t size;
};

/*
 * The state of protocol between us and a peer.
 */
struct protocol {
	int conn;
	enum protocol_state {
		PROTOCOL_IDLE,
		PROTOCOL_WAIT_HDR,
		PROTOCOL_WAIT_MSG,
		PROTOCOL_HAVE_MSG
	} state;
	struct msg_hdr hdr;
	struct buffer buf;
};

/*
 * A peer node in the Bitcoin P2P network.
 */
struct peer {
	enum peer_state {
		PEER_DISCONNECTED,
		PEER_VERSION,
		PEER_VERACK,
		PEER_CONNECTED
	} state;
	struct sockaddr_in6 addr;
	struct version_msg desc;
	struct protocol proto;
	struct peer *next;
};

inline void *
xcalloc(size_t n, size_t size)
{
	void *ptr = calloc(n, size);
	if (ptr == NULL) {
		syslog(LOG_CRIT, "failed to allocate %lu * %lu bytes of memory", n, size);
		exit(1);
	}
	return ptr;
}

inline void *
xrealloc(void *ptr, size_t size)
{
	void *tmp = realloc(ptr, size);
	if (tmp == NULL) {
		syslog(LOG_CRIT, "failed to reallocate %lu bytes of memory", size);
		exit(1);
	}
	return tmp;
}

inline void *
xmalloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr == NULL) {
		syslog(LOG_CRIT, "failed to allocate %lu bytes of memory", size);
		exit(1);
	}
	return ptr;
}

struct addrinfo *
discover_peers(void)
{
	const char *domain_name = "seed.bitcoin.sipa.be.";
	struct addrinfo *addrs = NULL, *ai;
	struct addrinfo hints;
	char buf[INET_ADDRSTRLEN | INET6_ADDRSTRLEN];
	const void *addr_ptr;

	memset(&hints, 0, sizeof hints);
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(domain_name, NULL, &hints, &addrs) == -1) {
		perror("getaddrinfo");
		return NULL;
	}

	printf("Discovering peers:\n");
	for (ai = addrs; ai != NULL; ai = ai->ai_next) {
		switch (ai->ai_family) {
		case AF_INET:
			addr_ptr = &((struct sockaddr_in *)ai->ai_addr)->sin_addr;
			break;
		case AF_INET6:
			addr_ptr = &((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr;
			break;
		default:
			continue;
		}
		printf(" %s\n",
		    inet_ntop(ai->ai_family, addr_ptr, buf, sizeof(buf)));
	}

	return addrs;
}

void
say_hello(int family, const struct sockaddr *sa, socklen_t salen, int peer)
{
	const char user_agent[] = "/btcnode:0.0.1/";
	struct version_msg ver_msg;
	int error;
	uint32_t i;

	(void)family;
	(void)salen;
	(void)sa;

	memset(&ver_msg, 0, sizeof(ver_msg));

	ver_msg.version = PROTOCOL_VERSION;
	ver_msg.services = 0;
	ver_msg.timestamp = time(NULL);

	ver_msg.recv_services = 0;
	if (family == AF_INET6) {
		memcpy(&ver_msg.recv_ip, &((struct sockaddr_in6 *)sa)->sin6_addr, sizeof(struct in6_addr));
	} else {
		memset(&ver_msg.recv_ip, 0, sizeof(ver_msg.recv_ip));
		memset(((uint8_t *)&ver_msg.recv_ip) + 4, 0xff, 2);
		i = htobe32(((struct sockaddr_in *)sa)->sin_addr.s_addr);
		memcpy(&ver_msg.recv_ip, &i, sizeof(i));
	}
	ver_msg.recv_port = MAINNET_PORT;

	ver_msg.trans_services = 0;
	memset(((uint8_t *)&ver_msg.trans_ip) + 4, 0xff, 2);
	i = INADDR_LOOPBACK;
	memcpy(&ver_msg.trans_ip, &i, sizeof(i));
	ver_msg.trans_port = MAINNET_PORT;

	ver_msg.nonce = 0;
	ver_msg.user_agent_len = sizeof(user_agent) - 1;
	strncpy(ver_msg.user_agent, user_agent, sizeof(ver_msg.user_agent));
	ver_msg.start_height = 0;
	ver_msg.relay = true;

	error = write_version_msg(&ver_msg, peer);
	if (error)
		(void)fprintf(stderr, "error: write_version_msg\n"), exit(1);

	memset(&ver_msg, 0, sizeof(ver_msg));
	error = read_version_msg(peer, &ver_msg);
	if (error)
		(void)fprintf(stderr, "error: read_version_msg: error %d\n", error), exit(1);

	printf("Peer's software: %*s\n", (int)ver_msg.user_agent_len, ver_msg.user_agent);
}

void
probe_peer(int family, const struct sockaddr *sa, socklen_t salen)
{
	switch (family) {
	case AF_INET:
		((struct sockaddr_in *)sa)->sin_port = htons(MAINNET_PORT);
		break;
	case AF_INET6:
		((struct sockaddr_in6 *)sa)->sin6_port = htons(MAINNET_PORT);
		break;
	default:
		return; // FIXME
	}

	int peer = socket(family, SOCK_STREAM, 0);
	if (peer == -1)
		perror("socket"), exit(1);

	if (connect(peer, sa, salen) == -1)
		perror("connect"), exit(1);

	say_hello(family, sa, salen, peer);
}

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	openlog("btcnode", LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_USER);

	srand(time(NULL) ^ getpid());

	struct addrinfo *addrs = discover_peers();
	for (struct addrinfo *ai = addrs; ai != NULL; ai = ai->ai_next) {
		if (ai->ai_family == AF_INET6)
			continue;
		probe_peer(ai->ai_family, ai->ai_addr, ai->ai_addrlen);
		break;
	}
	freeaddrinfo(addrs);

	return 0;
}
