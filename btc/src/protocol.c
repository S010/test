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

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "protocol.h"
#include "xmalloc.h"
#include "util.h"

uint8_t genesis_block_hash[32] = {
	0x6f, 0xe2, 0x8c, 0x0a,
	0xb6, 0xf1, 0xb3, 0x72,
	0xc1, 0xa6, 0xa2, 0x46,
	0xae, 0x63, 0xf7, 0x4f,
	0x93, 0x1e, 0x83, 0x65,
	0xe1, 0x5a, 0x08, 0x9c,
	0x68, 0xd6, 0x19, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

void
calc_double_hash(const void *in, size_t in_size, uint8_t *second/*[SHA256_DIGEST_LENGTH]*/)
{
	uint8_t first[SHA256_DIGEST_LENGTH];
	SHA256(in, in_size, first);
	SHA256(first, sizeof(first), second);
}

// FIXME Replace with proper non-blocking I/O
static int
write_all(int fd, void *data, size_t size)
{
	ssize_t n_wrote = 0;
	ssize_t n;
	while (n_wrote < (ssize_t)size) {
		n = write(fd, (uint8_t *)data + n_wrote, size - n_wrote);
		if (n == -1 && errno == EAGAIN)
			continue;
		else if (n == -1)
			return n_wrote;
		else
			n_wrote += n;
	}
	return n_wrote;
}

static int
calc_payload_checksum(const void *in, size_t in_size, uint8_t *out/*[4]*/)
{
	if (in_size == 0) {
		out[0] = 0x5d;
		out[1] = 0xf6;
		out[2] = 0xe0;
		out[3] = 0xe2;
		return 0;
	}

	uint8_t hash[SHA256_DIGEST_LENGTH];
	calc_double_hash(in, in_size, hash);
	memcpy(out, hash, 4);
	return 0;
}

static size_t
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

static size_t
calc_getheaders_msg_len(const struct getheaders_msg *msg)
{
	size_t size = 0;

	size += sizeof(msg->version);
	size += calc_varint_len(msg->hash_count);
	size += sizeof(msg->hash) * msg->hash_count;
	size += sizeof(msg->stop_hash);

	return size;
}

static size_t
calc_inv_msg_len(const struct inv_msg *msg)
{
	size_t size = 0;

	size += calc_varint_len(msg->count);
	size += msg->count * INV_VEC_LEN;

	return size;
}

// FIXME Carefully review this function, e.g. make sure out-of-bounds access is impossible.
// FIXME Perhaps it's better to just allocate space for the whole tree once?
// FIXME Perhaps it's better to use SHA256_Update() rather than SHA256() so we can avoid
//       copying a hash that's missing a pair?
void
calc_merkle_root(const struct block_msg *block, uint8_t *out /*[SHA256_DIGEST_LENGTH]*/)
{
	if (block->hdr.tx_count == 1) {
		memcpy(out, block->tx[0].id, SHA256_DIGEST_LENGTH);
		return;
	}

	uint64_t num_next = 0;
	uint64_t num_prev = block->hdr.tx_count;
	uint8_t *next_level = NULL;
	uint8_t *prev_level = NULL;
	uint8_t *tmp_ptr;

	char hash_str[HASH_LEN * 2 + 1];

	for (unsigned height = 0; num_next != 1; ++height) {
		syslog(LOG_DEBUG, "%s: height %u", __func__, height);
		if (height == 0) {
			// Prepopulate previous level with transaction IDs (i.e. transaction double-hashes).
			num_prev = block->hdr.tx_count + (block->hdr.tx_count & 1);
			prev_level = xrealloc(prev_level, num_prev * SHA256_DIGEST_LENGTH);
			syslog(LOG_DEBUG, "%s: prepopulating prev_level with %lu TXIDs", __func__, num_prev);
			for (uint64_t i = 0; i < num_prev; ++i) {
				uint64_t index = i;
				if (index == block->hdr.tx_count)
					index = block->hdr.tx_count - 1;
				memcpy(prev_level + i * SHA256_DIGEST_LENGTH, block->tx[index].id, SHA256_DIGEST_LENGTH);
				syslog(LOG_DEBUG, "%s: prev_level[%lu] <- tx[%lu].id{ %s }", __func__, i, index, strhash(block->tx[index].id, hash_str));
			}
			continue;
		}

		num_next = num_prev / 2; // Next level will contain twice less hashes...
		if (num_next != 1)
			num_next += num_next & 1; // We'll need to duplicate a hash if the number of hashes for next level is odd...
		syslog(LOG_DEBUG, "%s: there will be %lu hashes on next level", __func__, num_next);
		next_level = xrealloc(next_level, num_next * SHA256_DIGEST_LENGTH);
		for (uint64_t i = 0; i < num_prev; i += 2) {
			syslog(LOG_DEBUG, "%s: calculating next level combined hash %lu for prev. level pair at index %lu", __func__, i / 2, i);
			uint8_t *in_ptr = prev_level + i * SHA256_DIGEST_LENGTH;
			size_t in_len = 2 * SHA256_DIGEST_LENGTH;
			uint8_t *out_ptr = next_level + (i / 2) * SHA256_DIGEST_LENGTH;
			calc_double_hash(in_ptr, in_len, out_ptr);
		}
		if (num_next != 1 && ((num_prev / 2) & 1)) {
			syslog(LOG_DEBUG, "%s: padding next level with the last hash since it has odd number of hashes", __func__);
			memcpy(next_level + (num_next - 1) * SHA256_DIGEST_LENGTH, next_level + (num_next - 2) * SHA256_DIGEST_LENGTH, SHA256_DIGEST_LENGTH);
		}
		tmp_ptr = prev_level;
		prev_level = next_level;
		next_level = tmp_ptr;
		num_prev = num_next;
	}

	memcpy(out, prev_level, SHA256_DIGEST_LENGTH);
	free(prev_level);
}

static int
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
	calc_payload_checksum(payload, payload_size, hdr->checksum);

	return 0;
}

int
read_msg_hdr(int fd, struct msg_hdr *hdr)
{
	ssize_t n;
	uint8_t buf[MSG_HDR_LEN];
	uint8_t *in = buf;

	n = read(fd, buf, sizeof(buf));
	if (n == -1) {
		syslog(LOG_ERR, "%s: read failed, errno %d", __func__, errno);
		return n;
	} else if (n != sizeof(buf)) {
		syslog(LOG_ERR, "%s: short read of %d bytes", __func__, (int)n);
		return -1;
	}

	unmarshal_msg_hdr(in, hdr);

	return 0;
}

// FIXME Have a global string array which exactly matches the msg type enum.
static enum msg_types
get_msg_type(const struct msg_hdr *hdr)
{
	static const struct {
		const char *name;
		size_t len;
		enum msg_types type;
	} commands[] = {
		#define Y(x, y) { x, sizeof(x) - 1, y }
		Y("version", MSG_VERSION),
		Y("verack", MSG_VERACK),
		Y("addr", MSG_ADDR),
		Y("ping", MSG_PING),
		Y("pong", MSG_PONG),
		Y("addr", MSG_ADDR),
		Y("inv", MSG_INV),
		Y("headers", MSG_HEADERS), 
		Y("block", MSG_BLOCK),
		#undef Y
		{ NULL, 0, MSG_UNKNOWN }
	}, *cmd;

	for (cmd = commands; cmd->type != MSG_UNKNOWN; ++cmd) {
		if (memcmp(cmd->name, hdr->command, cmd->len) == 0)
			break;
	}
	return cmd->type;
}

size_t
marshal_version_msg(const struct version_msg *msg, uint8_t *out)
{
	size_t off = 0;
	off += marshal_u32(msg->version, out + off);
	off += marshal_u64(msg->services, out + off);
	off += marshal_u64(msg->timestamp, out + off);
	off += marshal_u64(msg->recv_addr.services, out + off);
	off += marshal_in6_addr(&msg->recv_addr.ip, out + off);
	off += marshal_u16(htons(msg->recv_addr.port), out + off);
	off += marshal_u64(msg->trans_addr.services, out + off);
	off += marshal_in6_addr(&msg->trans_addr.ip, out + off);
	off += marshal_u64(htons(msg->trans_addr.port), out + off);
	off += marshal_u64(msg->nonce, out + off);
	off += marshal_u8(msg->user_agent_len, out + off);
	off += marshal_bytes(msg->user_agent, msg->user_agent_len, out + off);
	off += marshal_u32(msg->start_height, out + off);
	off += marshal_u8(msg->relay, out + off);
	return off;
}

int
unmarshal_version_msg(const struct msg_hdr *hdr, const uint8_t *payload, struct version_msg *msg)
{
	const uint8_t *in = payload;

	in += unmarshal_u32(in, (uint32_t *)&msg->version);
	in += unmarshal_u64(in, &msg->services);
	in += unmarshal_u64(in, &msg->timestamp);
	in += unmarshal_u64(in, &msg->recv_addr.services);
	in += unmarshal_bytes(in, sizeof(msg->recv_addr.ip), &msg->recv_addr.ip);
	in += unmarshal_u16(in, &msg->recv_addr.port);
	in += unmarshal_u64(in, &msg->trans_addr.services);
	in += unmarshal_bytes(in, sizeof(msg->trans_addr.ip), &msg->trans_addr.ip);
	in += unmarshal_u16(in, &msg->trans_addr.port);
	in += unmarshal_u64(in, &msg->nonce);
	in += unmarshal_u8(in, &msg->user_agent_len);
	if (msg->user_agent_len >= 0xfd) {
		syslog(LOG_ERR, "%s: user agent string is longer than we can handle", __func__);
		return -1;
	}
	if ((hdr->payload_size - (in - payload) - 4 - 1) != msg->user_agent_len) {
		syslog(LOG_ERR, "%s: the user agent length is wrong at %u bytes", __func__, msg->user_agent_len);
		return -1;
	}
	in += unmarshal_bytes(in, msg->user_agent_len, msg->user_agent);
	in += unmarshal_u32(in, (uint32_t *)&msg->start_height);
	in += unmarshal_u8(in, &msg->relay);

	msg->type = MSG_VERSION;

	return 0;
}

size_t
marshal_msg_hdr(const struct msg_hdr *hdr, uint8_t *out)
{
	size_t off = 0;
	off += marshal_bytes(hdr->start, sizeof(hdr->start), out + off);
	off += marshal_bytes(hdr->command, sizeof(hdr->command), out + off);
	off += marshal_u32(hdr->payload_size, out + off);
	off += marshal_bytes(hdr->checksum, sizeof(hdr->checksum), out + off);
	return off;
}

size_t
unmarshal_msg_hdr(const uint8_t *in, struct msg_hdr *hdr)
{
	size_t off = 0;
	off += unmarshal_bytes(in + off, sizeof(hdr->start), hdr->start);
	off += unmarshal_bytes(in + off, sizeof(hdr->command), hdr->command);
	off += unmarshal_u32(in + off, &hdr->payload_size);
	off += unmarshal_bytes(in + off, sizeof(hdr->checksum), hdr->checksum);
	return off;
}

size_t
marshal_ping_msg(const struct ping_msg *msg, uint8_t *out)
{
	return marshal_u64(msg->nonce, out);
}

int
unmarshal_ping_msg(const struct msg_hdr *hdr, const uint8_t *payload, struct ping_msg *msg)
{
	(void)hdr;
	unmarshal_u64(payload, &msg->nonce);
	msg->type = MSG_PING;
	return 0;
}

size_t
marshal_in6_addr(const struct in6_addr *addr, uint8_t *out)
{
	for (int i = 15; i >= 0; --i) {
		*out++ = ((const uint8_t *)addr)[i];
	}
	return 16;
}

int
write_version_msg(const struct version_msg *msg, int fd)
{
	struct msg_hdr hdr;
	size_t msg_len = calc_version_msg_len(msg);
	uint8_t buf[MSG_HDR_LEN + msg_len];

	marshal_version_msg(msg, buf + MSG_HDR_LEN);
	fill_msg_hdr(MAINNET_START, "version", buf + MSG_HDR_LEN, msg_len, &hdr);
	marshal_msg_hdr(&hdr, buf);

	int n = write_all(fd, buf, sizeof(buf));
	if (n < 0)
		return n;
	else if ((size_t)n != sizeof(buf))
		return -1;
	return 0;
}

int
read_version_msg(int fd, struct version_msg *msg)
{
	ssize_t n;
	struct msg_hdr hdr;

	n = read_msg_hdr(fd, &hdr);
	if (n != 0) {
		syslog(LOG_ERR, "%s: read_msg_hdr failed", __func__);
		return n;
	}

	if (hdr.payload_size < VERSION_MSG_MIN_LEN) {
		syslog(LOG_ERR, "%s: payload_size is too small at %u bytes", __func__, (unsigned)hdr.payload_size);
		return 1;
	}
	if (hdr.payload_size > 256) {
		syslog(LOG_ERR, "%s: payload_size too great at %u bytes", __func__, (unsigned)hdr.payload_size);
		return 2;
	}

	uint8_t buf[hdr.payload_size];
	uint8_t checksum[4];
	uint8_t *in = buf;

	n = read(fd, buf, hdr.payload_size);
	if (n == -1) {
		syslog(LOG_ERR, "%s: read failed, errno %d", __func__, errno);
		return -1;
	}
	if ((size_t)n != hdr.payload_size) {
		syslog(LOG_ERR, "%s: short read of %zd bytes", __func__, n);
		return -1;
	}
	// FIXME Use the proper unmarshal function.
	in += unmarshal_u32(in, (uint32_t *)&msg->version);
	in += unmarshal_u64(in, &msg->services);
	in += unmarshal_u64(in, &msg->timestamp);
	in += unmarshal_u64(in, &msg->recv_addr.services);
	in += unmarshal_bytes(in, sizeof(msg->recv_addr.ip), &msg->recv_addr.ip);
	in += unmarshal_u16(in, &msg->recv_addr.port);
	in += unmarshal_u64(in, &msg->trans_addr.services);
	in += unmarshal_bytes(in, sizeof(msg->trans_addr.ip), &msg->trans_addr.ip);
	in += unmarshal_u16(in, &msg->trans_addr.port);
	in += unmarshal_u64(in, &msg->nonce);
	in += unmarshal_u8(in, &msg->user_agent_len);
	if (msg->user_agent_len >= 0xfd) {
		syslog(LOG_ERR, "%s: user agent string is longer than we can handle", __func__);
		return 3;
	}
	if ((hdr.payload_size - (in - buf) - 4 - 1) != msg->user_agent_len) {
		syslog(LOG_ERR, "%s: the user agent length is wrong at %u bytes", __func__, msg->user_agent_len);
		return 4;
	}
	in += unmarshal_bytes(in, msg->user_agent_len, msg->user_agent);
	in += unmarshal_u32(in, (uint32_t *)&msg->start_height);
	in += unmarshal_u8(in, &msg->relay);

	// FIXME Checksum is already calculated in read_message().
	calc_payload_checksum(buf, hdr.payload_size, checksum);

	if (memcmp(checksum, hdr.checksum, 4) != 0) {
		syslog(LOG_ERR, "%s: the payload chekcsum appears to be wrong", __func__);
		return 5;
	}

	return 0;
}

int
unmarshal_msg(const struct buffer *buf, const struct msg_hdr *hdr, union message **msg)
{
	if (hdr->payload_size > MSG_HDR_MAX_PAYLOAD_SIZE) {
		syslog(LOG_ERR, "%s: payload size is greater than max. allowed size at %u", __func__, hdr->payload_size);
		return -1;
	}

	uint8_t checksum[4];
	const uint8_t *payload = buf->bytes + MSG_HDR_LEN; // FIXME Pass direct pointer?
	calc_payload_checksum(payload, hdr->payload_size, checksum);
	if (memcmp(hdr->checksum, checksum, sizeof(checksum)) != 0) {
		syslog(LOG_ERR, "%s: the payload checksum is wrong", __func__);
		return -1;
	}

	enum msg_types msg_type = get_msg_type(hdr);
	int error = 0;
	switch (msg_type) {
	case MSG_VERSION:
		*msg = xmalloc(sizeof((*msg)->version));
		error = unmarshal_version_msg(hdr, payload, &(*msg)->version);
		syslog(LOG_DEBUG, "%s: got version message", __func__);
		break;
	case MSG_VERACK:
		// FIXME
		*msg = xmalloc(sizeof((*msg)->type));
		(*msg)->type = MSG_VERACK;
		syslog(LOG_DEBUG, "%s: got verack message", __func__);
		break;
	case MSG_PING:
	case MSG_PONG:
		syslog(LOG_DEBUG, "%s: got %s message", __func__, msg_type == MSG_PING ? "ping" : "pong");
		*msg = xmalloc(sizeof((*msg)->ping));
		error = unmarshal_ping_msg(hdr, payload, &(*msg)->ping);
		(*msg)->type = msg_type;
		break;
	case MSG_ADDR:
		// FIXME
		*msg = xmalloc(sizeof((*msg)->type));
		(*msg)->type = MSG_UNKNOWN;
		syslog(LOG_DEBUG, "%s: ignored addr message", __func__);
		break;
	case MSG_INV:
		syslog(LOG_DEBUG, "%s: got inv message", __func__);
		error = unmarshal_inv_msg(hdr, payload, (struct inv_msg **)msg);
		break;
	case MSG_HEADERS:
		syslog(LOG_DEBUG, "%s: got headers message", __func__);
		error = unmarshal_headers_msg(hdr, payload, (struct headers_msg **)msg);
		break;
	case MSG_BLOCK:
		syslog(LOG_DEBUG, "%s: got block message", __func__);
		// FIXME Pass proper pointers or handle it inside functions.
		error = unmarshal_block_msg(hdr, payload, (struct block_msg **)msg);
		break;
	default:
		*msg = xmalloc(sizeof((*msg)->type));
		(*msg)->type = MSG_UNKNOWN;
		syslog(LOG_DEBUG, "%s: got an unknown messsage, command field is '%*s'",
		    __func__, (int)strnlen(hdr->command, sizeof(hdr->command)), hdr->command);
		break;
	}

	if (error)
		syslog(LOG_ERR, "%s: failed to unmarshal message of type %d", __func__, msg_type);

	return error;
}

int
write_verack_msg(int fd)
{
	struct msg_hdr hdr;
	uint8_t buf[MSG_HDR_LEN];

	fill_msg_hdr(MAINNET_START, "verack", NULL, 0, &hdr);
	marshal_msg_hdr(&hdr, buf);

	int n = write_all(fd, buf, sizeof(buf));
	if (n < 0)
		return n;
	else if ((size_t)n != sizeof(buf))
		return -1;
	return 0;
}

int
read_verack_msg(int fd)
{
	static const char verack[] = "verack";
	struct msg_hdr hdr;
	int error;

	error = read_msg_hdr(fd, &hdr);
	if (error) {
		syslog(LOG_ERR, "%s: failed to read verack message header", __func__);
		return error;
	}

	if (memcmp(hdr.command, verack, sizeof(verack) - 1) == 0)
		return 0;
	else {
		syslog(LOG_ERR, "%s: the received message header is not verack", __func__);
		return 1;
	}
}

int
write_pong_msg(const struct ping_msg *msg, int fd)
{
	uint8_t buf[MSG_HDR_LEN + PING_MSG_LEN];
	struct msg_hdr hdr;

	marshal_ping_msg(msg, buf + MSG_HDR_LEN);
	fill_msg_hdr(MAINNET_START, "pong", buf + MSG_HDR_LEN, PING_MSG_LEN, &hdr);
	marshal_msg_hdr(&hdr, buf);

	int n = write_all(fd, buf, sizeof(buf));
	if (n < 0)
		return n;
	else if ((size_t)n != sizeof(buf))
		return -1;
	return 0;
}

size_t
marshal_getheaders_msg(const struct getheaders_msg *msg, uint8_t *out)
{
	size_t off = 0;

	off += marshal_u32(msg->version, out + off);
	off += marshal_varint(msg->hash_count, out + off);
	off += marshal_bytes(msg->hash, sizeof(msg->hash) * msg->hash_count, out + off);
	off += marshal_bytes(msg->stop_hash, sizeof(msg->stop_hash), out + off);

	return off;
}

int
write_getheaders_msg(int fd, const struct getheaders_msg *msg)
{
	struct msg_hdr hdr;
	// FIXME Allocate using dynamic memory?
	size_t msg_len = calc_getheaders_msg_len(msg);
	uint8_t buf[MSG_HDR_LEN + msg_len];

	marshal_getheaders_msg(msg, buf + MSG_HDR_LEN);
	fill_msg_hdr(MAINNET_START, "getheaders", buf + MSG_HDR_LEN, msg_len, &hdr);
	marshal_msg_hdr(&hdr, buf);

	int n = write_all(fd, buf, sizeof(buf));
	if (n < 0)
		return n;
	else if ((size_t)n != sizeof(buf))
		return -1;
	return 0;
}

int
unmarshal_inv_msg(const struct msg_hdr *hdr, const uint8_t *payload, struct inv_msg **out)
{
	if (hdr->payload_size < INV_MSG_MIN_LEN) {
		syslog(LOG_ERR, "%s: payload size is too small for inv message at %u bytes", __func__, hdr->payload_size);
		return -1;
	}

	struct inv_msg *msg = xmalloc(sizeof(*msg));

	size_t off = 0;
	off += unmarshal_varint(payload + off, &msg->count);

	size_t expected_size = calc_varint_len(msg->count) + msg->count * INV_VEC_LEN;
	if (hdr->payload_size != expected_size) {
		syslog(LOG_ERR, "%s: payload size (%u) doesn't match the expected size (%zu)", __func__, hdr->payload_size, expected_size);
		free(msg);
		return -1;
	}

	msg = xrealloc(msg, sizeof(*msg) + sizeof(struct inv_vec) * msg->count);
	for (size_t i = 0; i < msg->count; ++i)
		off += unmarshal_inv_vec(payload + off, &msg->inv_vec[i]);

	msg->type = MSG_INV;

	*out = msg;

	return 0;
}

int
write_getdata_msg(const struct inv_msg *msg, int fd)
{
	struct msg_hdr hdr;
	size_t msg_len = calc_inv_msg_len(msg);
	uint8_t buf[MSG_HDR_LEN + msg_len];

	marshal_inv_msg(msg, buf + MSG_HDR_LEN);
	fill_msg_hdr(MAINNET_START, "getdata", buf + MSG_HDR_LEN, msg_len, &hdr);
	marshal_msg_hdr(&hdr, buf);

	int n = write_all(fd, buf, sizeof(buf));
	if (n < 0)
		return n;
	else if ((size_t)n != sizeof(buf))
		return -1;
	return 0;
}

size_t
marshal_block_hdr(const struct block_hdr *hdr, uint8_t *out)
{
	size_t off = 0;
	off += marshal_u32(hdr->version, out + off);
	off += marshal_bytes(hdr->prev_hash, sizeof(hdr->prev_hash), out + off);
	off += marshal_bytes(hdr->merkle_root, sizeof(hdr->prev_hash), out + off);
	off += marshal_u32(hdr->timestamp, out + off);
	off += marshal_u32(hdr->bits, out + off);
	off += marshal_u32(hdr->nonce, out + off);
	off += marshal_varint(hdr->tx_count, out + off);
	return off;
}

size_t
unmarshal_block_hdr(const uint8_t *in, struct block_hdr *hdr)
{
	size_t off = 0;
	off += unmarshal_u32(in + off, &hdr->version);
	off += unmarshal_bytes(in + off, sizeof(hdr->prev_hash), hdr->prev_hash);
	off += unmarshal_bytes(in + off, sizeof(hdr->merkle_root), hdr->merkle_root);
	off += unmarshal_u32(in + off, &hdr->timestamp);
	off += unmarshal_u32(in + off, &hdr->bits);
	off += unmarshal_u32(in + off, &hdr->nonce);
	off += unmarshal_varint(in + off, &hdr->tx_count);
	return off;
}

int
unmarshal_headers_msg(const struct msg_hdr *hdr, const uint8_t *payload, struct headers_msg **out)
{
	if (hdr->payload_size < HEADERS_MSG_MIN_LEN) {
		syslog(LOG_ERR, "%s: payload size is too small at %u bytes", __func__, hdr->payload_size);
		return -1;
	}

	struct headers_msg *msg = xmalloc(sizeof(*msg));
	size_t off = unmarshal_varint(payload, &msg->count);

	size_t expected_size = calc_varint_len(msg->count) + msg->count * BLOCK_HDR_MIN_LEN;
	if (hdr->payload_size != expected_size) {
		free(msg);
		syslog(LOG_ERR, "%s: payload size (%u) is smaller than expected (%zu)", __func__, hdr->payload_size, expected_size);
		return -1;
	}

	msg = xrealloc(msg, sizeof(*msg) + msg->count * sizeof(struct block_hdr));
	for (size_t i = 0; i < msg->count; ++i)
		off += unmarshal_block_hdr(payload + off, &msg->headers[i]);

	msg->type = MSG_HEADERS;

	*out = msg;

	return 0;
}

void
calc_block_hdr_hash(const struct block_hdr *hdr, uint8_t *out /*[32]*/)
{
	uint8_t buf[BLOCK_HDR_MAX_LEN];
	marshal_block_hdr(hdr, buf);
	calc_double_hash(buf, BLOCK_HDR_MIN_LEN - 1, out);
}

static size_t
unmarshal_tx_input(const uint8_t *in, struct tx_input **out)
{
	struct tx_input *tx_in = xmalloc(sizeof(*tx_in));

	// FIXME Check boundaries.

	size_t off = 0;
	off += unmarshal_bytes(in + off, sizeof(tx_in->prev_output.hash), tx_in->prev_output.hash);
	off += unmarshal_u32(in + off, &tx_in->prev_output.index);
	off += unmarshal_varint(in + off, &tx_in->script_len);
	tx_in = xrealloc(tx_in, sizeof(*tx_in) + tx_in->script_len);
	off += unmarshal_bytes(in + off, tx_in->script_len, tx_in->script);
	off += unmarshal_u32(in + off, &tx_in->seq);

	*out = tx_in;

	return off;
}

static size_t
unmarshal_tx_output(const uint8_t *in, struct tx_output **out)
{
	struct tx_output *tx_out = xmalloc(sizeof(*tx_out));

	// FIXME Check boundaries.

	size_t off = 0;
	off += unmarshal_u64(in + off, &tx_out->value);
	off += unmarshal_varint(in + off, &tx_out->pk_script_len);
	tx_out = xrealloc(tx_out, sizeof(*tx_out) + tx_out->pk_script_len);
	off += unmarshal_bytes(in + off, tx_out->pk_script_len, tx_out->pk_script);

	*out = tx_out;

	return off;
}

size_t
unmarshal_tx_msg(const uint8_t *in, struct tx_msg *tx)
{
	size_t off = 0;
	off += unmarshal_u32(in + off, &tx->version);
	off += unmarshal_varint(in + off, &tx->in_count);
	tx->in = xmalloc(tx->in_count * sizeof(struct tx_input *));
	for (size_t i = 0; i < tx->in_count; ++i) {
		off += unmarshal_tx_input(in + off, &tx->in[i]);
	}
	off += unmarshal_varint(in + off, &tx->out_count);
	tx->out = xmalloc(tx->out_count * sizeof(struct tx_output *));
	for (size_t i = 0; i < tx->out_count; ++i) {
		off += unmarshal_tx_output(in + off, &tx->out[i]);
	}
	off += unmarshal_u32(in + off, &tx->locktime);
	calc_double_hash(in, off, tx->id);
	return off;
}

int
unmarshal_block_msg(const struct msg_hdr *hdr, const uint8_t *payload, struct block_msg **out)
{
	if (hdr->payload_size < HEADERS_MSG_MIN_LEN) {
		syslog(LOG_ERR, "%s: payload size is too small at %u bytes", __func__, hdr->payload_size);
		return -1;
	}

	struct block_msg *msg = xmalloc(sizeof(*msg));
	size_t off = 0;
	off += unmarshal_block_hdr(payload, &msg->hdr);
	if (msg->hdr.tx_count > BLOCK_MSG_MAX_TXNS) {
		free(msg);
		syslog(LOG_ERR, "%s: block contains %lu txns, whereas a maximum of %d is allowed", __func__, msg->hdr.tx_count, BLOCK_MSG_MAX_TXNS);
		return -1;
	}

	msg = xrealloc(msg, sizeof(*msg) + msg->hdr.tx_count * sizeof(struct tx_msg));
	for (uint64_t i = 0; i < msg->hdr.tx_count; ++i) {
		off += unmarshal_tx_msg(payload + off, &msg->tx[i]);
	}

	msg->type = MSG_BLOCK;

	*out = msg;

	return 0;
}

void
free_block_msg(struct block_msg *block)
{
	for (size_t i = 0; i < block->hdr.tx_count; ++i) {
		for (size_t j = 0; j < block->tx[i].in_count; ++j)
			free(block->tx[i].in[j]);
		free(block->tx[i].in);
		for (size_t j = 0; j < block->tx[i].out_count; ++j)
			free(block->tx[i].out[j]);
		free(block->tx[i].out);
	}
	free(block);
}

int
read_block_msg(int fd, struct block_msg **out)
{
	ssize_t n;
	struct msg_hdr hdr;

	n = read_msg_hdr(fd, &hdr);
	if (n != 0) {
		syslog(LOG_ERR, "%s: read_msg_hdr failed", __func__);
		return n;
	}

	if (hdr.payload_size < BLOCK_MSG_MIN_LEN) {
		syslog(LOG_ERR, "%s: payload_size is too small at %u bytes", __func__, (unsigned)hdr.payload_size);
		return 1;
	}

	uint8_t *buf = xmalloc(hdr.payload_size);

	n = read(fd, buf, hdr.payload_size);
	if (n == -1) {
		free(buf);
		syslog(LOG_ERR, "%s: read failed, errno %d", __func__, errno);
		return -1;
	}
	if ((size_t)n != hdr.payload_size) {
		free(buf);
		syslog(LOG_ERR, "%s: short read of %zd bytes", __func__, n);
		return -1;
	}

	int error = unmarshal_block_msg(&hdr, buf, out);

	free(buf);

	return error;
}
