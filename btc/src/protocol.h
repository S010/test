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
#define HASH_LEN 32

typedef uint64_t varint_t;

extern uint8_t genesis_block_hash[32];

enum msg_types {
	MSG_UNKNOWN,
	MSG_VERSION,
	MSG_VERACK,
	MSG_PING,
	MSG_PONG,
	MSG_ADDR,
	MSG_INV,
	MSG_HEADERS,
	MSG_BLOCK
};
struct msg_hdr {
	uint8_t start[4];
	char command[12];
	uint32_t payload_size;
	uint8_t checksum[4];
};
#define MSG_HDR_LEN ((size_t)(4 + 12 + sizeof(uint32_t) + 4))
#define MSG_HDR_MAX_PAYLOAD_SIZE ((1024 * 1024) * 4)

struct netaddr {
	uint32_t timestamp;
	uint64_t services;
	struct in6_addr ip; // 16
	uint16_t port;
};

struct version_msg {
	enum msg_types type;
	int32_t version; // 4 bytes
	uint64_t services; // 8
	uint64_t timestamp; // 8

	struct netaddr recv_addr;
	struct netaddr trans_addr;

	uint64_t nonce; // 8

	// FIXME Allow varint as per protocol.
	uint8_t user_agent_len; // 1+
	char user_agent[0xfd - 1]; // ???

	int32_t start_height; // 4

	uint8_t relay; // 1
};
#define VERSION_MSG_MIN_LEN 86

struct ping_msg { // Also represents 'pong' message.
	enum msg_types type;
	uint64_t nonce;
};
#define PING_MSG_LEN sizeof(uint64_t)

struct addr_msg {
	enum msg_types type;
	uint64_t count;
	struct netaddr addr_list[];
};
#define ADDR_MSG_MAX_LIST 1000
#define ADDR_MSG_LIST_ELEM_LEN (sizeof(uint32_t) + sizeof(uint64_t) + 16 + 2)
#define ADDR_MSG_MAX_LEN (9 + ADDR_MSG_MAX_LIST * ADDR_MSG_LIST_ELEM_LEN)

struct getheaders_msg { // Also represents 'getblocks' message.
	enum msg_types type;
	uint32_t version;
	varint_t hash_count;
	uint8_t stop_hash[32];
	uint8_t hash[32];
};

struct block_hdr {
	uint32_t version;
	uint8_t prev_hash[32];
	uint8_t merkle_root[32];
	uint32_t timestamp;
	uint32_t bits;
	uint32_t nonce;
	varint_t tx_count; // Always zero in headers message.
};
#define BLOCK_HDR_MIN_LEN (4 + 2*32 + 3*4 + 1)
#define BLOCK_HDR_MAX_LEN (4 + 2*32 + 3*4 + 9)
struct headers_msg {
	enum msg_types type;
	varint_t count;
	struct block_hdr headers[];
};
#define HEADERS_MSG_MIN_LEN (1 + sizeof(struct block_hdr))

struct inv_vec {
	uint32_t type;
	uint8_t hash[32];
};
enum {
	INV_TYPE_ERROR,
	INV_TYPE_TX,
	INV_TYPE_BLOCK,
	INV_TYPE_FILTERED_BLOCK,
	INV_TYPE_CMPCT_BLOCK
};
#define INV_VEC_LEN 36
struct inv_msg { // Also represents 'getdata' message.
	enum msg_types type;
	varint_t count;
	struct inv_vec inv_vec[];
};
#define INV_MSG_MIN_LEN (1 + INV_VEC_LEN)
// FIXME Enforce the max. count limit in the code.
#define INV_MSG_MAX_COUNT 50000

struct outpoint {
	uint8_t hash[32];
	uint32_t index;
};
// =============================
// tx_input
// =============================
// siz name            type
// -----------------------------
// 36  previous_output  outpoint
// 1+  script length    var_int
// ?   signature script uchar[]
// 4   sequence         uint32_t
// -----------------------------
struct tx_input {
	struct outpoint prev_output;
	uint32_t seq;
	varint_t script_len;
	uint8_t script[];
};
// =============================
// tx_output
// =============================
// siz name             type
// -----------------------------
// 8   value            uint64_t
// 1+  pk_script length var_int
// ?   pk_script        uchar[]
// -----------------------------
struct tx_output {
	uint64_t value;
	varint_t pk_script_len;
	uint8_t pk_script[];
};
// ==================================
// tx_msg
// ==================================
// siz name         type
// ----------------------------------
// 4   version      int32_t (signed!)
// 1+  tx_in count  var_int
// 41+ tx_in        tx_in[]
// 1+  tx_out count var_int
// 9+  tx_out       tx_out[]
// 4   lock_time    uint32_t
// ----------------------------------
struct tx_msg {
	uint32_t version;
	uint8_t id[32];
	varint_t in_count;
	struct tx_input **in;
	varint_t out_count;
	struct tx_output **out;
	uint32_t locktime;
};
struct block_msg {
	enum msg_types type;
	struct block_hdr hdr;
	struct tx_msg tx[];
};
#define BLOCK_MSG_MIN_LEN BLOCK_HDR_MIN_LEN
#define BLOCK_MSG_MAX_TXNS 2000

/*
 * Can hold any Bitcoin protocol message.
 */
union message {
	enum msg_types type;
	struct version_msg version;
	struct ping_msg ping;
	struct addr_msg addr;
	struct getheaders_msg getheaders;
	struct inv_msg inv;
	struct headers_msg headers;
	struct block_msg block;
};

/*
 * A resizable data buffer.
 */
struct buffer {
	union {
		size_t len;
		size_t idx;
	};
	size_t size;
	uint8_t bytes[];
};

/*
 * The state of protocol between us and a peer.
 */
struct protocol {
	int conn;
	// FIXME Replace WAIT_HDR, WAIT_MSG with WAIT_<particular_msg>
	enum protocol_state {
		WAIT_HEADER,
		WAIT_MESSAGE,
		HAVE_MESSAGE
	} state;
	struct buffer *buf;
	struct msg_hdr hdr;
	union message *msg;
};

inline size_t
marshal_u8(uint8_t i, uint8_t *out)
{
	*out++ = i;
	return 1;
}

inline size_t
marshal_u16(uint16_t i, uint8_t *out)
{
	*out++ = i & 0xff;
	*out++ = i >> 8;
	return 2;
}

inline size_t
marshal_u32(uint32_t i, uint8_t *out)
{
	*out++ = i & 0xff;
	*out++ = (i >> 8) & 0xff;
	*out++ = (i >> 16) & 0xff;
	*out++ = (i >> 24) & 0xff;
	return 4;
}

inline size_t
marshal_u64(uint64_t i, uint8_t *out)
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

inline size_t
marshal_varint(uint64_t i, uint8_t *out)
{
	size_t off = 1;
	if (i < 0xFDull) {
		*out = (uint8_t)i;
	} else if (i <= 0xFFFFull) {
		*out++ = 0xFD;
		off += marshal_u16(i, out);
	} else if (i <= 0xFFFFFFFFull) {
		*out++ = 0xFE;
		off += marshal_u32(i, out);
	} else {
		*out++ = 0xFF;
		off += marshal_u64(i, out);
	}
	return off;
}

inline size_t
calc_varint_len(uint64_t i)
{
	if (i < 0xFDull)
		return 1;
	else if (i <= 0xFFFFull)
		return 3;
	else if (i <= 0xFFFFFFFFull)
		return 5;
	else
		return 9;
}

inline size_t
marshal_inv_vec(const struct inv_vec *v, uint8_t *out)
{
	size_t off = 0;
	off += marshal_u32(v->type, out + off);
	off += marshal_bytes(v->hash, sizeof(v->hash), out + off);
	return off;
}

inline size_t
marshal_inv_msg(const struct inv_msg *msg, uint8_t *out)
{
	size_t off = 0;
	off += marshal_varint(msg->count, out + off);
	for (size_t i = 0; i < msg->count; ++i)
		off += marshal_inv_vec(&msg->inv_vec[i], out + off);
	return off;
}

size_t marshal_version_msg(const struct version_msg *msg, uint8_t *out);
size_t marshal_msg_hdr(const struct msg_hdr *hdr, uint8_t *out);
size_t marshal_in6_addr(const struct in6_addr *addr, uint8_t *out);
size_t marshal_ping_msg(const struct ping_msg *msg, uint8_t *out);
size_t marshal_getheaders_msg(const struct getheaders_msg *msg, uint8_t *out);
size_t marshal_block_hdr(const struct block_hdr *hdr, uint8_t *out);

inline size_t
unmarshal_bytes(const void *in, size_t len, void *out)
{
	memcpy(out, in, len);
	return len;
}

inline size_t
unmarshal_u32(const uint8_t *in, uint32_t *out)
{
	memcpy(out, in, 4);
	*out = le32toh(*out);
	return 4;
}

inline size_t
unmarshal_u64(const uint8_t *in, uint64_t *out)
{
	memcpy(out, in, 8);
	*out = le64toh(*out);
	return 8;
}

inline size_t
unmarshal_u16(const uint8_t *in, uint16_t *out)
{
	memcpy(out, in, 2);
	return 2;
}

inline size_t
unmarshal_u8(const uint8_t *in, uint8_t *out)
{
	*out = *in++;
	return 1;
}

// FIXME Check boundaries.
inline size_t
unmarshal_varint(const uint8_t *in, uint64_t *out)
{
	uint16_t u16;
	uint32_t u32;
	uint8_t mark = *in++;
	if (mark < 0xFD) {
		*out = mark;
		return 1;
	} else if (mark == 0xFD) {
		// FIXME Convert to little endian on big endian machines.
		unmarshal_u16(in, &u16);
		*out = u16;
		return 3;
	} else if (mark == 0xFE) {
		unmarshal_u32(in, &u32);
		*out = u32;
		return 5;
	} else {
		unmarshal_u64(in, out);
		return 9;
	}
}

inline size_t
unmarshal_inv_vec(const uint8_t *in, struct inv_vec *out)
{
	size_t off = 0;
	off += unmarshal_u32(in + off, &out->type);
	off += unmarshal_bytes(in + off, sizeof(out->hash), out->hash);
	return off;
}

size_t unmarshal_msg_hdr(const uint8_t *in, struct msg_hdr *hdr);
int unmarshal_inv_msg(const struct msg_hdr *hdr, const uint8_t *payload, struct inv_msg **out);
int unmarshal_headers_msg(const struct msg_hdr *hdr, const uint8_t *payload, struct headers_msg **out);
int unmarshal_block_msg(const struct msg_hdr *hdr, const uint8_t *payload, struct block_msg **out);
int unmarshal_msg(const struct buffer *, const struct msg_hdr *, union message **);

int write_version_msg(const struct version_msg *msg, int fd);
int write_verack_msg(int fd);
int write_getheaders_msg(int fd, const struct getheaders_msg *msg);
int write_getdata_msg(const struct inv_msg *msg, int fd);
// FIXME Use consistent argument order: fd should be the last.
int write_pong_msg(const struct ping_msg *msg, int fd);

int read_version_msg(int fd, struct version_msg *msg);
int read_verack_msg(int fd);
int read_block_msg(int fd, struct block_msg **out);

void calc_block_hdr_hash(const struct block_hdr *hdr, uint8_t *out /*[32]*/);

void calc_merkle_root(const struct block_msg *block, uint8_t *out);

void free_block_msg(struct block_msg *block);

inline void free_msg(union message *msg)
{
	if (msg != NULL && msg->type == MSG_BLOCK)
		free_block_msg((struct block_msg *)msg);
	else
		free(msg);
}

#endif
