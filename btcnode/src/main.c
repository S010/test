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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
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

#include "protocol.h"

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
		printf("%s\n",
		    inet_ntop(ai->ai_family, addr_ptr, buf, sizeof(buf)));
	}

	// FIXME
	//freeaddrinfo(addrs);

	return addrs;
}

void
say_hello(int family, const struct sockaddr *sa, socklen_t salen, int peer)
{
	const char user_agent[] = "btcnode";
	struct version_msg ver_msg;
	int error;
	struct in6_addr in6_addr = IN6ADDR_LOOPBACK_INIT;

	(void)family;
	(void)salen;
	(void)sa;

	memset(&ver_msg, 0, sizeof(ver_msg));

	ver_msg.version = PROTOCOL_VERSION;
	ver_msg.services = 0;
	ver_msg.timestamp = time(NULL);
	ver_msg.recv_services = NODE_NETWORK;
	// FIXME Convert IPv4 address to IPv6 if needed
	//marshal(&((struct sockaddr_in6 *)sa)->sin6_addr, ver_msg.recv_ip);
	marshal(&in6_addr, ver_msg.recv_ip);
	ver_msg.recv_port = MAINNET_PORT;
	ver_msg.trans_services = ver_msg.services;
	marshal(&in6_addr, ver_msg.trans_ip);
	ver_msg.trans_port = MAINNET_PORT; // FIXME Port we connected _from_?
	ver_msg.nonce = ((uint64_t)rand() << 32) | (uint64_t)rand();
	ver_msg.user_agent_len = sizeof(user_agent) - 1;
	ver_msg.user_agent = user_agent;
	ver_msg.start_height = 0;
	ver_msg.relay = true;

	error = write_version_msg(&ver_msg, peer);
	if (error)
		(void)fprintf(stderr, "error: write_version_msg\n"), exit(1);
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

	sleep(5);
}

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	srand(time(NULL) ^ getpid());

	int fd;

	fd = open("btcnode.out", O_WRONLY | O_TRUNC | O_CREAT, 0600);
	if (fd == -1)
		return 1;

	struct addrinfo *addrs = discover_peers();
	for (struct addrinfo *ai = addrs; ai != NULL; ai = ai->ai_next) {
		if (ai->ai_family != AF_INET6)
			continue;
		// FIXME Convert peer addr to IPv6 if it's IPv4 in say_hello
		//probe_peer(ai->ai_family, ai->ai_addr, ai->ai_addrlen);
		say_hello(ai->ai_family, ai->ai_addr, ai->ai_addrlen, fd);
		break;
	}
	freeaddrinfo(addrs);

	close(fd);

	return 0;
}
