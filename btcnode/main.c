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

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <openssl/sha.h>

// FIXME
#define syslog(level, ...) do { printf(__VA_ARGS__); putchar('\n'); } while (0)

#define PROTO_VERSION 70014

#define MAINNET_START ((const uint8_t[]){ 0xf9, 0xbe, 0xb4, 0xd9 })
#define MAINNET_PORT 8333

struct msg_hdr {
	uint8_t start[4];
	char command[12];
	uint32_t payload_size;
	uint8_t checksum[4];
};
#define MSG_HDR_LEN ((size_t)(4 + 12 + sizeof(uint32_t) + sizeof(uint8_t)))

struct version_msg {
	int32_t version;
	uint64_t services;
	uint64_t timestamp;

	uint64_t recv_services;
	uint8_t recv_ip[16];
	uint16_t recv_port;

	uint64_t trans_services;
	uint8_t trans_ip[16];
	uint16_t trans_port;

	uint8_t nonce;

	uint16_t user_agent_len;
	char *user_agent;

	int32_t start_height;

	bool relay;
};

int
calc_msg_checksum(const void *in, size_t in_size, uint8_t *out/*[4]*/)
{
	SHA256_CTX ctx;
	uint8_t buf[2][SHA_DIGEST_LENGTH];
	int error;

	error = SHA256_Init(&ctx);
	if (error)
		return error;

	for (int i = 0; i < 2; ++i) {
		error = SHA256_Update(&ctx, in, in_size);
		if (error)
			return error;

		error = SHA256_Final(buf[i], &ctx);
		if (error)
			return error;

		in = buf[i];
		in_size = sizeof(buf[i]);
	}

	memcpy(out, buf[1], 4);

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
	    sizeof(uint8_t) + // recv_ip
	    sizeof(uint16_t) + // recv_port

	    sizeof(uint64_t) + // trans_services
	    sizeof(uint8_t) + // trans_ip
	    sizeof(uint16_t) + // trans_port

	    sizeof(uint8_t) + // nonce

	    !!(msg->user_agent_len & 0xff) +
	    !!(msg->user_agent_len & 0xff00) +

	    msg->user_agent_len +

	    sizeof(int32_t) + // start_height

	    1; // relay

	return len;
}

int
marshal_version_msg(const struct version_msg *msg, uint8_t *out)
{
	uint32_t h, l;
	uint64_t qword;

	#define APPEND_UINT16(name) \
		do { \
			*((uint16_t *)out) = htons(msg->name); \
			out += sizeof(msg->name); \
		} while (0)

	#define APPEND_INT32(name) \
		do { \
			*((int32_t *)out) = htonl(msg->name); \
			out += sizeof(msg->name); \
		} while (0)

	#define APPEND_UINT64(name) \
		do { \
			h = htonl(msg->name >> 32); \
			l = htonl(msg->name); \
			qword = (((uint64_t)l) << 32) | (uint64_t)h; \
			*((uint64_t *)out) = qword; \
			out += sizeof(qword); \
		} while (0)

	#define APPEND_IP(name) \
		do { \
			memcpy(out, msg->name, sizeof(msg->name)); \
			out += sizeof(msg->name); \
		} while (0)

	APPEND_INT32(version);

	APPEND_UINT64(services);
	APPEND_UINT64(timestamp);

	APPEND_UINT64(recv_services);
	APPEND_IP(recv_ip);
	APPEND_UINT16(recv_port);

	APPEND_UINT64(trans_services);
	APPEND_IP(trans_ip);
	APPEND_UINT16(trans_port);

	*out++ = msg->nonce;

	if (msg->user_agent_len & 0xff00)
		*out++ = msg->user_agent_len >> 8;
	if (msg->user_agent_len & 0xff)
		*out++ = msg->user_agent_len & 0xff;

	if (msg->user_agent_len > 0)
		memcpy(out, msg->user_agent, msg->user_agent_len);

	APPEND_INT32(start_height);

	*out++ = msg->relay;

	return 0;
}

int
marshal_msg_hdr(const struct msg_hdr *hdr, uint8_t *out)
{
	memcpy(out, hdr->start, sizeof(hdr->start));
	out += sizeof(hdr->start);

	memcpy(out, hdr->command, sizeof(hdr->command));
	out += sizeof(hdr->command);

	*((uint32_t *)out) = htonl(hdr->payload_size);
	out += sizeof(hdr->payload_size);

	memcpy(out, hdr->checksum, sizeof(hdr->checksum));
	out += sizeof(hdr->checksum);

	return 0;
}

#define marshal(x, y) \
	_Generic((x), \
		struct msg_hdr *: marshal_msg_hdr, \
		const struct msg_hdr *: marshal_msg_hdr, \
		struct version_msg *: marshal_version_msg, \
		const struct version_msg *: marshal_version_msg \
	)((x), (y))

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

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	openlog("btcnode", LOG_PID, LOG_LOCAL0);
	syslog(LOG_NOTICE, "started");

	int fd;

	fd = open("btcnode.out", O_WRONLY | O_TRUNC | O_CREAT, 0600);
	if (fd == -1)
		return 1;

	struct version_msg msg;

	memset(&msg, 0, sizeof(msg));

	msg.version = PROTO_VERSION;
	msg.services = 1;

	write_version_msg(&msg, fd);

	close(fd);

	return 0;
}
