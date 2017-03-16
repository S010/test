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

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <arpa/inet.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <endian.h>

#define PROTOCOL_VERSION 70014

#define MAINNET_START ((const uint8_t[]){ 0xf9, 0xbe, 0xb4, 0xd9 })
#define MAINNET_PORT 8333

#define NODE_NETWORK 0x01

enum msg_types {
	MSG_UNKNOWN,
	MSG_VERSION,
	MSG_VERACK,
	MSG_PING,
	MSG_PONG,
	MSG_ADDR
};

struct msg_hdr {
	uint8_t start[4];
	char command[12];
	uint32_t payload_size;
	uint8_t checksum[4];
};
#define MSG_HDR_LEN ((size_t)(4 + 12 + sizeof(uint32_t) + 4))

struct netaddr {
	uint32_t timestamp;
	uint64_t services;
	struct in6_addr ip; // 16
	uint16_t port;
};

struct version_msg {
	int32_t version; // 4 bytes
	uint64_t services; // 8
	uint64_t timestamp; // 8

	struct netaddr recv_addr;
	struct netaddr trans_addr;

	uint64_t nonce; // 8

	uint8_t user_agent_len; // 1+
	char user_agent[0xfd - 1]; // ???

	int32_t start_height; // 4

	uint8_t relay; // 1
};
#define MIN_VERSION_MSG_LEN 86

struct ping_msg {
	uint64_t nonce;
};
#define PING_MSG_LEN sizeof(uint64_t)

struct addr_msg {
	uint64_t count;
	struct netaddr addr_list[];
};
#define MAX_ADDR_MSG_LIST 1000
#define ADDR_MSG_LIST_ELEM_LEN (sizeof(uint32_t) + sizeof(uint64_t) + 16 + 2)
#define MAX_ADDR_MSG_LEN (9 + MAX_ADDR_MSG_LIST * ADDR_MSG_LIST_ELEM_LEN)

union message {
	struct version_msg version;
	struct ping_msg ping;
	struct addr_msg addr;
};

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

inline size_t
marshal_uint8(uint8_t i, uint8_t *out)
{
	*out++ = i;
	return 1;
}

inline size_t
marshal_uint16(uint16_t i, uint8_t *out)
{
	*out++ = i & 0xff;
	*out++ = i >> 8;
	return 2;
}

inline size_t
marshal_uint32(uint32_t i, uint8_t *out)
{
	*out++ = i & 0xff;
	*out++ = (i >> 8) & 0xff;
	*out++ = (i >> 16) & 0xff;
	*out++ = (i >> 24) & 0xff;
	return 4;
}

inline size_t
marshal_uint64(uint64_t i, uint8_t *out)
{
	int n = 8;
	while (n--) {
		*out++ = i & 0xff;
		i >>= 8;
	}
	return 8;
}

inline size_t
marshal_bytes(const void *in, size_t in_size, void *out)
{
	memcpy(out, in, in_size);
	return in_size;
}

size_t marshal_version_msg(const struct version_msg *msg, uint8_t *out);
size_t marshal_msg_hdr(const struct msg_hdr *hdr, uint8_t *out);
size_t marshal_in6_addr(const struct in6_addr *addr, uint8_t *out);
size_t marshal_ping_msg(const struct ping_msg *msg, uint8_t *out);

#define marshal(x, y) \
	_Generic((x), \
		struct msg_hdr *: marshal_msg_hdr, \
		const struct msg_hdr *: marshal_msg_hdr, \
		struct version_msg *: marshal_version_msg, \
		const struct version_msg *: marshal_version_msg, \
		struct in6_addr *: marshal_in6_addr, \
		const struct in6_addr *: marshal_in6_addr, \
		struct ping_msg *: marshal_ping_msg, \
		const struct ping_msg *: marshal_ping_msg, \
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
unmarshal_bytes(const void *in, size_t len, void *out)
{
	memcpy(out, in, len);
	return len;
}

inline size_t
unmarshal_uint32(const uint8_t *in, uint32_t *out)
{
	memcpy(out, in, 4);
	*out = le32toh(*out);
	return 4;
}

inline size_t
unmarshal_uint64(const uint8_t *in, uint64_t *out)
{
	memcpy(out, in, 8);
	*out = le64toh(*out);
	return 8;
}

inline size_t
unmarshal_uint16(const uint8_t *in, uint16_t *out)
{
	memcpy(out, in, 2);
	return 2;
}

inline size_t
unmarshal_uint8(const uint8_t *in, uint8_t *out)
{
	*out = *in++;
	return 1;
}

#define unmarshal(x, y) \
	_Generic((y), \
		int64_t *: unmarshal_uint64, \
		uint64_t *: unmarshal_uint64, \
		int32_t *: unmarshal_uint32, \
		uint32_t *: unmarshal_uint32, \
		int16_t *: unmarshal_uint16, \
		uint16_t *: unmarshal_uint16, \
		uint8_t *: unmarshal_uint8 \
	)((x), (y))

int write_version_msg(const struct version_msg *msg, int fd);
int read_version_msg(int fd, struct version_msg *msg);

int write_verack_msg(int fd);
int read_verack_msg(int fd);

int read_message(int fd, enum msg_types *type, union message **msg);

int write_pong_msg(int fd, struct ping_msg *msg);

#endif
