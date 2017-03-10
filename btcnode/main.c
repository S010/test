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

#include <openssl/sha.h>

// FIXME
#define syslog(level, ...) do { printf(__VA_ARGS__); putchar('\n'); } while (0)

#define PROTOCOL_VERSION 70014

#define MAINNET_START ((const uint8_t[]){ 0xf9, 0xbe, 0xb4, 0xd9 })
#define MAINNET_PORT 8333

#define NODE_NETWORK 0x01


struct msg_hdr {
	uint8_t start[4];
	char command[12];
	uint32_t payload_size;
	uint8_t checksum[4];
};
#define MSG_HDR_LEN ((size_t)(4 + 12 + sizeof(uint32_t) + 4))

struct version_msg {
	int32_t version;
	uint64_t services;
	uint64_t timestamp;

	uint64_t recv_services;
	uint8_t recv_ip[16]; // FIXME Make this and trans_ip in6_addr
	uint16_t recv_port;

	uint64_t trans_services;
	uint8_t trans_ip[16];
	uint16_t trans_port;

	uint64_t nonce;

	uint8_t user_agent_len;
	const char *user_agent;

	int32_t start_height;

	uint8_t relay;
};

int
calc_msg_checksum(const void *in, size_t in_size, uint8_t *out/*[4]*/)
{
	uint8_t hash[2][SHA_DIGEST_LENGTH];

	for (int i = 0; i < 2; ++i) {
		SHA256(in, in_size, hash[i]);
		/*
		printf("SHA256[%d]: ", i);
		for (int j = 0; j < SHA_DIGEST_LENGTH; ++j) {
			printf("%02x", hash[i][j]);
		}
		putchar('\n');
		*/
		in = hash[i];
		in_size = sizeof(hash[i]);
	}

	memcpy(out, hash[1], 4);

	return 0;
}

size_t
calc_version_msg_len(const struct version_msg *msg)
{
	size_t len;

	// FIXME do sizeof of actual members
	len =
	    sizeof(int32_t) + // version
	    sizeof(uint64_t) + // services
	    sizeof(uint64_t) + // timestamp

	    sizeof(uint64_t) + // recv_services
	    16 + // recv_ip
	    sizeof(uint16_t) + // recv_port

	    sizeof(uint64_t) + // trans_services
	    16 + // trans_ip
	    sizeof(uint16_t) + // trans_port

	    sizeof(uint64_t) + // nonce

	    1 + msg->user_agent_len + 

	    sizeof(int32_t) + // start_height

	    1; // relay

	return len;
}

#define marshal(x, y) \
	_Generic((x), \
		struct msg_hdr *: marshal_msg_hdr, \
		const struct msg_hdr *: marshal_msg_hdr, \
		struct version_msg *: marshal_version_msg, \
		const struct version_msg *: marshal_version_msg, \
		struct in6_addr *: marshal_in6_addr, \
		const struct in6_addr *: marshal_in6_addr, \
		int32_t: marshal_uint32, \
		uint32_t: marshal_uint32, \
		int16_t: marshal_uint16, \
		uint16_t: marshal_uint16, \
		int64_t: marshal_uint64, \
		uint64_t: marshal_uint64, \
		int8_t: marshal_uint8, \
		uint8_t: marshal_uint8 \
	)((x), (y))

inline size_t
marshal_uint8(uint8_t i, uint8_t *out)
{
	*out++ = i;
	return 1;
}

inline size_t
marshal_uint16(uint16_t i, uint8_t *out)
{
	*out++ = (i & 0xff00) >> 8;
	*out++ = i & 0xff;
	return 2;
}

inline size_t
marshal_uint32(uint32_t i, uint8_t *out)
{
	*out++ = (i >> 24) & 0xff;
	*out++ = (i >> 16) & 0xff;
	*out++ = (i >> 8) & 0xff;
	*out++ = i & 0xff;
	return 4;
}

inline size_t
marshal_uint64(uint64_t i, uint8_t *out)
{
	int n = 7;
	while (n >= 0) {
		*out++ = i >> (8 * n--);
	}
	return 8;
}

#define marshal_bytes(in, size, out) \
	marshal_bytes_any((const uint8_t *)in, size, out)

inline size_t
marshal_bytes_any(const uint8_t *in, size_t in_size, uint8_t *out)
{
	memcpy(out, in, in_size);
	return in_size;
}

size_t marshal_msg_hdr(const struct msg_hdr *hdr, uint8_t *out);
size_t marshal_in6_addr(const struct in6_addr *addr, uint8_t *out);

size_t
marshal_version_msg(const struct version_msg *msg, uint8_t *out)
{
	size_t off = 0;
	off += marshal(msg->version, out + off);
	off += marshal(msg->services, out + off);
	off += marshal(msg->timestamp, out + off);
	off += marshal(msg->recv_services, out + off);
	off += marshal_bytes(msg->recv_ip, sizeof(msg->recv_ip), out + off);
	off += marshal(msg->recv_port, out + off);
	off += marshal(msg->trans_services, out + off);
	off += marshal_bytes(msg->trans_ip, sizeof(msg->trans_ip), out + off);
	off += marshal(msg->trans_port, out + off);
	off += marshal(msg->nonce, out + off);
	off += marshal(msg->user_agent_len, out + off);
	off += marshal_bytes(msg->user_agent, msg->user_agent_len, out + off);
	off += marshal(msg->start_height, out + off);
	off += marshal(msg->relay, out + off);
	return off;
}

size_t
marshal_msg_hdr(const struct msg_hdr *hdr, uint8_t *out)
{
	size_t off = 0;
	off += marshal_bytes(hdr->start, sizeof(hdr->start), out + off);
	off += marshal_bytes(hdr->command, sizeof(hdr->command), out + off);
	off += marshal(hdr->payload_size, out + off);
	off += marshal_bytes(hdr->checksum, sizeof(hdr->checksum), out + off);
	return off;
}

size_t
marshal_in6_addr(const struct in6_addr *addr, uint8_t *out)
{
	for (int i = 15; i >= 0; --i)
		*out++ = ((const uint8_t *)addr)[i];
	return 16;
}

int
fill_msg_hdr(
    const uint8_t *start,
    const char *command,
    const void *payload,
    size_t payload_size,
    struct msg_hdr *hdr)
{
	memcpy(hdr->start, start, sizeof(hdr->start));
	memset(hdr->command, 0, sizeof(hdr->command));
	strcpy(hdr->command, command);
	hdr->payload_size = payload_size;
	calc_msg_checksum(payload, payload_size, hdr->checksum);

	return 0;
}

int
write_version_msg(const struct version_msg *msg, int fd)
{
	struct msg_hdr hdr;
	size_t msg_len = calc_version_msg_len(msg);
	uint8_t buf[MSG_HDR_LEN + msg_len];

	marshal(msg, buf + MSG_HDR_LEN);
	fill_msg_hdr(MAINNET_START, "version", buf + MSG_HDR_LEN, msg_len, &hdr);
	marshal(&hdr, buf);

	int n = write(fd, buf, sizeof(buf));
	if (n < 0)
		return n;
	else if ((size_t)n != sizeof(buf))
		return -1;
	return 0;
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

	openlog("btcnode", LOG_PID, LOG_LOCAL0);
	syslog(LOG_NOTICE, "started");

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
