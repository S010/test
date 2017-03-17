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

	uint8_t hash[2][SHA256_DIGEST_LENGTH];
	for (int i = 0; i < 2; ++i) {
		SHA256(in, in_size, hash[i]);
		in = hash[i];
		in_size = SHA256_DIGEST_LENGTH;
	}
	memcpy(out, hash[1], 4);

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

static int
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

	unmarshal(in, hdr);

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
	off += marshal(msg->version, out + off);
	off += marshal(msg->services, out + off);
	off += marshal(msg->timestamp, out + off);
	off += marshal(msg->recv_addr.services, out + off);
	off += marshal(&msg->recv_addr.ip, out + off);
	off += marshal(htons(msg->recv_addr.port), out + off);
	off += marshal(msg->trans_addr.services, out + off);
	off += marshal(&msg->trans_addr.ip, out + off);
	off += marshal(htons(msg->trans_addr.port), out + off);
	off += marshal(msg->nonce, out + off);
	off += marshal(msg->user_agent_len, out + off);
	off += marshal_bytes(msg->user_agent, msg->user_agent_len, out + off);
	off += marshal(msg->start_height, out + off);
	off += marshal(msg->relay, out + off);
	return off;
}

int
unmarshal_version_msg(const struct msg_hdr *hdr, const uint8_t *payload, struct version_msg *msg)
{
	const uint8_t *in = payload;

	in += unmarshal(in, (uint32_t *)&msg->version);
	in += unmarshal(in, &msg->services);
	in += unmarshal(in, &msg->timestamp);
	in += unmarshal(in, &msg->recv_addr.services);
	in += unmarshal_bytes(in, sizeof(msg->recv_addr.ip), &msg->recv_addr.ip);
	in += unmarshal(in, &msg->recv_addr.port);
	in += unmarshal(in, &msg->trans_addr.services);
	in += unmarshal_bytes(in, sizeof(msg->trans_addr.ip), &msg->trans_addr.ip);
	in += unmarshal(in, &msg->trans_addr.port);
	in += unmarshal(in, &msg->nonce);
	in += unmarshal(in, &msg->user_agent_len);
	if (msg->user_agent_len >= 0xfd) {
		syslog(LOG_ERR, "%s: user agent string is longer than we can handle", __func__);
		return -1;
	}
	if ((hdr->payload_size - (in - payload) - 4 - 1) != msg->user_agent_len) {
		syslog(LOG_ERR, "%s: the user agent length is wrong at %u bytes", __func__, msg->user_agent_len);
		return -1;
	}
	in += unmarshal_bytes(in, msg->user_agent_len, msg->user_agent);
	in += unmarshal(in, (uint32_t *)&msg->start_height);
	in += unmarshal(in, &msg->relay);

	return 0;
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
unmarshal_msg_hdr(const uint8_t *in, struct msg_hdr *hdr)
{
	size_t off = 0;
	off += unmarshal_bytes(in + off, sizeof(hdr->start), hdr->start);
	off += unmarshal_bytes(in + off, sizeof(hdr->command), hdr->command);
	off += unmarshal(in + off, &hdr->payload_size);
	off += unmarshal_bytes(in + off, sizeof(hdr->checksum), hdr->checksum);
	return off;
}

size_t
marshal_ping_msg(const struct ping_msg *msg, uint8_t *out)
{
	return marshal(msg->nonce, out);
}

int
unmarshal_ping_msg(const struct msg_hdr *hdr, uint8_t *payload, struct ping_msg *msg)
{
	(void)hdr;
	unmarshal(payload, &msg->nonce);
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
	if (n != hdr.payload_size) {
		syslog(LOG_ERR, "%s: short read of %u bytes", __func__, (unsigned)n);
		return -1;
	}
	in += unmarshal(in, (uint32_t *)&msg->version);
	in += unmarshal(in, &msg->services);
	in += unmarshal(in, &msg->timestamp);
	in += unmarshal(in, &msg->recv_addr.services);
	in += unmarshal_bytes(in, sizeof(msg->recv_addr.ip), &msg->recv_addr.ip);
	in += unmarshal(in, &msg->recv_addr.port);
	in += unmarshal(in, &msg->trans_addr.services);
	in += unmarshal_bytes(in, sizeof(msg->trans_addr.ip), &msg->trans_addr.ip);
	in += unmarshal(in, &msg->trans_addr.port);
	in += unmarshal(in, &msg->nonce);
	in += unmarshal(in, &msg->user_agent_len);
	if (msg->user_agent_len >= 0xfd) {
		syslog(LOG_ERR, "%s: user agent string is longer than we can handle", __func__);
		return 3;
	}
	if ((hdr.payload_size - (in - buf) - 4 - 1) != msg->user_agent_len) {
		syslog(LOG_ERR, "%s: the user agent length is wrong at %u bytes", __func__, msg->user_agent_len);
		return 4;
	}
	in += unmarshal_bytes(in, msg->user_agent_len, msg->user_agent);
	in += unmarshal(in, (uint32_t *)&msg->start_height);
	in += unmarshal(in, &msg->relay);

	calc_payload_checksum(buf, hdr.payload_size, checksum);

	if (memcmp(checksum, hdr.checksum, 4) != 0) {
		syslog(LOG_ERR, "%s: the payload chekcsum appears to be wrong", __func__);
		return 5;
	}

	return 0;
}

int
read_message(int fd, enum msg_types *type, union message **msg)
{
	struct msg_hdr hdr;
	ssize_t n, n_read = 0;
	uint8_t checksum[4];

	n = read_msg_hdr(fd, &hdr);
	if (n != 0) {
		syslog(LOG_ERR, "%s: failed to read message header", __func__);
		return -1;
	}

	if (hdr.payload_size > MSG_HDR_MAX_PAYLOAD_SIZE) {
		syslog(LOG_ERR, "%s: payload size is greater than max. allowed size at %u", __func__, hdr.payload_size);
		return -1;
	}

	*type = get_msg_type(&hdr);
	uint8_t *payload = xmalloc(hdr.payload_size);

	for (n_read = 0; n_read != hdr.payload_size; n_read += n) {
		n = read(fd, payload + n_read, hdr.payload_size - n_read);
		if (n < 0) {
			free(payload);
			syslog(LOG_ERR, "%s: failed to read message payload from peer, errno %d", __func__, errno);
			return -1;
		} else if (n == 0) {
			free(payload);
			syslog(LOG_ERR, "%s: failed to read message payload from peer, short read", __func__);
			return -1;
		}
	}

	/*
	printf("%s: received %*s message payload of size %u (%x):\n", __func__, (int)strlen(hdr.command), hdr.command, hdr.payload_size, hdr.payload_size);
	hexdump(payload, hdr.payload_size);
	fflush(stdout);
	*/

	calc_payload_checksum(payload, hdr.payload_size, checksum);
	if (memcmp(hdr.checksum, checksum, sizeof(checksum)) != 0) {
		free(payload);
		syslog(LOG_ERR, "%s: the payload checksum is wrong", __func__);
		return -1;
	}

	int error = 0;
	switch (*type) {
	case MSG_VERSION:
		*msg = xmalloc(sizeof((*msg)->version));
		error = unmarshal_version_msg(&hdr, payload, &(*msg)->version);
		syslog(LOG_DEBUG, "%s: got version message", __func__);
		break;
	case MSG_VERACK:
		syslog(LOG_DEBUG, "%s: got verack message", __func__);
		free(payload);
		break;
	case MSG_PING:
	case MSG_PONG:
		syslog(LOG_DEBUG, "%s: got ping/pong message", __func__);
		*msg = xmalloc(sizeof((*msg)->ping));
		error = unmarshal_ping_msg(&hdr, payload, &(*msg)->ping);
		break;
	case MSG_ADDR:
		// FIXME
		*type = MSG_UNKNOWN;
		syslog(LOG_DEBUG, "%s: ignored addr message", __func__);
		break;
	case MSG_INV:
		syslog(LOG_DEBUG, "%s: got inv message", __func__);
		error = unmarshal_inv_msg(&hdr, payload, (struct inv_msg **)msg);
		break;
	case MSG_HEADERS:
		syslog(LOG_DEBUG, "%s: got headers message", __func__);
		error = unmarshal_headers_msg(&hdr, payload, (struct headers_msg **)msg);
		break;
	default:
		syslog(LOG_DEBUG, "%s: got an unknown messsage, command field is '%*s'", __func__, (int)strnlen(hdr.command, sizeof(hdr.command)), hdr.command);
		break;
	}

	free(payload);

	if (error)
		syslog(LOG_ERR, "%s: failed to unmarshal message type %d", __func__, *type);

	return error;
}

int
write_verack_msg(int fd)
{
	struct msg_hdr hdr;
	uint8_t buf[MSG_HDR_LEN];

	fill_msg_hdr(MAINNET_START, "verack", NULL, 0, &hdr);
	marshal(&hdr, buf);

	int n = write(fd, buf, sizeof(buf));
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

	marshal(msg, buf + MSG_HDR_LEN);
	fill_msg_hdr(MAINNET_START, "pong", buf + MSG_HDR_LEN, PING_MSG_LEN, &hdr);
	marshal(&hdr, buf);

	int n = write(fd, buf, sizeof(buf));
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

	off += marshal(msg->version, out + off);
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

	marshal(msg, buf + MSG_HDR_LEN);
	fill_msg_hdr(MAINNET_START, "getheaders", buf + MSG_HDR_LEN, msg_len, &hdr);
	marshal(&hdr, buf);

	int n = write(fd, buf, sizeof(buf));
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
		syslog(LOG_ERR, "%s: payload size (%u) doesn't match the expected size (%lu)", __func__, hdr->payload_size, expected_size);
		free(msg);
		return -1;
	}

	msg = xrealloc(msg, sizeof(*msg) + sizeof(struct inv_vec) * msg->count);
	for (size_t i = 0; i < msg->count; ++i)
		off += unmarshal(payload + off, &msg->inv_vec[i]);

	*out = msg;

	return 0;
}

int
write_getdata_msg(const struct inv_msg *msg, int fd)
{
	struct msg_hdr hdr;
	size_t msg_len = calc_inv_msg_len(msg);
	uint8_t buf[MSG_HDR_LEN + msg_len];

	marshal(msg, buf + MSG_HDR_LEN);
	fill_msg_hdr(MAINNET_START, "getdata", buf + MSG_HDR_LEN, msg_len, &hdr);
	marshal(&hdr, buf);

	int n = write(fd, buf, sizeof(buf));
	if (n < 0)
		return n;
	else if ((size_t)n != sizeof(buf))
		return -1;
	return 0;
}

size_t
unmarshal_block_hdr(const uint8_t *in, struct block_hdr *hdr)
{
	size_t off = 0;

	off += unmarshal(in + off, &hdr->version);
	off += unmarshal_bytes(in + off, sizeof(hdr->prev_hash), hdr->prev_hash);
	off += unmarshal_bytes(in + off, sizeof(hdr->merkle_root), hdr->merkle_root);
	off += unmarshal(in + off, &hdr->timestamp);
	off += unmarshal(in + off, &hdr->bits);
	off += unmarshal(in + off, &hdr->nonce);
	off += unmarshal(in + off, &hdr->tx_count);

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

	size_t expected_size = calc_varint_len(msg->count) + msg->count * BLOCK_HDR_LEN;
	if (hdr->payload_size != expected_size) {
		free(msg);
		syslog(LOG_ERR, "%s: payload size (%u) is smaller than expected (%lu)", __func__, hdr->payload_size, expected_size);
		return -1;
	}

	msg = xrealloc(msg, sizeof(*msg) + msg->count * sizeof(struct block_hdr));
	for (size_t i = 0; i < msg->count; ++i)
		off += unmarshal_block_hdr(payload + off, &msg->headers[i]);

	*out = msg;

	return 0;
}
