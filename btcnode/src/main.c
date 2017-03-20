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
#include "peer.h"
#include "xmalloc.h"

static int
request_headers(int fd)
{
	struct getheaders_msg msg;

	msg.version = PROTOCOL_VERSION;
	msg.hash_count = 1;
	memset(msg.stop_hash, 0, sizeof(msg.stop_hash));
	memcpy(msg.hash, genesis_block_hash, sizeof(genesis_block_hash));

	return write_getheaders_msg(fd, &msg);
}

static int
handle_ping_message(struct peer *peer, union message *msg)
{
	int error = write_pong_msg(&msg->ping, peer->proto.conn);
	if (error) {
		syslog(LOG_ERR, "%s: failed to reply to peer's ping", __func__);
		return -1;
	}
	return 0;
}

static int
handle_inv_message(struct peer *peer, union message *msg)
{
	(void)peer;
	syslog(LOG_DEBUG, "%s: got inventory from peer, %lu elems:", __func__, msg->inv.count);
	for (size_t i = 0; i < msg->inv.count; ++i) {
		const uint8_t *p = msg->inv.inv_vec[i].hash;
		syslog(LOG_DEBUG, "%s:   type %d, %02x%02x%02x%02x...", __func__, msg->inv.inv_vec[i].type,  p[0], p[1], p[2], p[3]);
	}
	return 0;
}

static int
request_block(struct peer *peer, uint8_t hash[32])
{
	struct inv_msg *msg = xmalloc(sizeof(*msg) + sizeof(struct inv_vec));
	msg->count = 1;
	msg->inv_vec[0].type = INV_TYPE_BLOCK;
	memcpy(msg->inv_vec[0].hash, hash, 32); // FIXME Remove magic numbers.

	int error = write_getdata_msg(msg, peer->proto.conn);
	free(msg);
	return error;
}

static int
handle_headers_message(struct peer *peer, union message *msg)
{
	(void)peer;
	syslog(LOG_DEBUG, "%s: got %lu block headers from peer", __func__, msg->headers.count);
	uint8_t hash[32];
	bool all_correct = true;
	for (size_t i = 0; i < msg->headers.count; ++i) {
		const struct block_hdr *h = &msg->headers.headers[i];
		syslog(LOG_DEBUG, "%s: block header #%lu: "
		    "{ ver=%x, prev=%02x%02x%02x%02x..., merkle=%02x%02x%02x%02x..., time=%08x, bits=%u, nonce=%08x }",
		    __func__, i,
		    h->version,
		    h->prev_hash[0], h->prev_hash[1], h->prev_hash[2], h->prev_hash[3],
		    h->merkle_root[0], h->merkle_root[1], h->merkle_root[2], h->merkle_root[3],
		    h->timestamp, h->bits, h->nonce);
		if (i > 0) {
			bool is_correct = memcmp(hash, h->prev_hash, sizeof(hash)) == 0;
			all_correct = all_correct && is_correct;
			if (!is_correct) {
				syslog(LOG_ERR, "%s: hash of block header %lu is incorrect", __func__, i - 1);
			}
		}
		calc_block_hdr_hash(h, hash);
		const size_t fetch_block_interval = 50;
		if ((i % fetch_block_interval) == 0) {
			syslog(LOG_DEBUG, "%s: requesting full block #%lu", __func__, i);
			int error = request_block(peer, hash);
			if (error) {
				syslog(LOG_ERR, "%s: failed to request block identified by hash %02x%02x%02x%02x...", __func__, hash[0], hash[1], hash[2], hash[3]);
			}
		}
	}
	if (all_correct) {
		syslog(LOG_DEBUG, "%s: hashes of all block headers are correct", __func__);
		return 0;
	} else {
		syslog(LOG_ERR, "%s: hashes of some of block headers are incorrect", __func__);
		return -1;
	}
}

static int
handle_block_message(struct peer *peer, union message *msg)
{
	struct block_msg *block = &msg->block;
	uint8_t hash[HASH_LEN];
	uint8_t merkle_root[HASH_LEN];

	(void)peer;

	calc_block_hdr_hash(&block->hdr, hash);
	calc_merkle_root(block, merkle_root);

	syslog(LOG_DEBUG, "%s: block %02x%02x%02x%02x..., %lu txns", __func__, hash[0], hash[1], hash[2], hash[3], block->hdr.tx_count);

	if (memcmp(block->hdr.merkle_root, merkle_root, HASH_LEN) != 0) {
		syslog(LOG_ERR, "%s:       merkle root mismatch, is %02x%02x%02x%02x..., should be %02x%02x%02x%02x...",
		    __func__,
		    block->hdr.merkle_root[0], block->hdr.merkle_root[1], block->hdr.merkle_root[2], block->hdr.merkle_root[3],
		    merkle_root[0], merkle_root[1], merkle_root[2], merkle_root[3]);
	}

	for (size_t i = 0; i < block->hdr.tx_count; ++i) {
		struct tx_msg *tx = &block->tx[i];
		syslog(LOG_DEBUG, "%s:       tx #%lu: ver %u, %lu in, %lu out, locktime %u", __func__, i, tx->version, tx->in_count, tx->out_count, tx->locktime);
		for (size_t j = 0; j < tx->in_count; ++j) {
			uint8_t *out_hash = tx->in[j]->prev_output.hash;
			syslog(LOG_DEBUG, "%s:       tx #%lu: input #%lu from %02x%02x%02x%02x...", __func__, i, j, out_hash[0], out_hash[1], out_hash[2], out_hash[3]);
		}
		for (size_t j = 0; j < tx->out_count; ++j) {
			uint8_t *scr = tx->out[j]->pk_script;
			syslog(LOG_DEBUG, "%s:       tx #%lu: output #%lu, value %lu, script len %lu, script %02x%02x%02x%02x...", __func__, i, j, tx->out[j]->value, tx->out[j]->pk_script_len, scr[0], scr[1], scr[2], scr[3]);
		}
	}
	return 0;
}

static int
handle_message(struct peer *peer, enum msg_types msg_type, union message *msg)
{
	int error = 0;
	switch (msg_type) {
	case MSG_PING:
		error = handle_ping_message(peer, msg);
		if (request_headers(peer->proto.conn) != 0) {
			error = true;
			syslog(LOG_ERR, "%s: failed to ask peer for block headers", __func__);
		}
		break;
	case MSG_INV:
		error = handle_inv_message(peer, msg);
		break;
	case MSG_HEADERS:
		error = handle_headers_message(peer, msg);
		break;
	case MSG_BLOCK:
		error = handle_block_message(peer, msg);
		break;
	default:
		break;
	}
	if (msg_type == MSG_BLOCK) {
		free_block_msg(&msg->block);
	} else {
		free(msg);
	}
	return error;
}

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	openlog("btcnode", LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_USER);

	srand(time(NULL) ^ getpid());

	struct peer *peers = discover_peers();
	print_peers(peers);

	for (struct peer *peer = peers; peer != NULL; peer = peer->next) {
		int error = peer_connect(peer);
		if (error) {
			continue;
		}

		enum msg_types type;
		union message *msg = NULL;
		while (read_message(peer->proto.conn, &type, &msg) == 0) {
			handle_message(peer, type, msg);
			msg = NULL;
		}
		break;
	}

	return 0;
}
