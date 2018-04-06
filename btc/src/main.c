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
#include <sys/epoll.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
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
#include "util.h"

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
		const struct block_hdr *blk_hdr = &msg->headers.headers[i];
		syslog(LOG_DEBUG, "%s: block header #%zu: "
		    "{ ver=%x, prev=%02x%02x%02x%02x..., merkle=%02x%02x%02x%02x..., time=%08x, bits=%u, nonce=%08x }",
		    __func__, i,
		    blk_hdr->version,
		    blk_hdr->prev_hash[0], blk_hdr->prev_hash[1], blk_hdr->prev_hash[2], blk_hdr->prev_hash[3],
		    blk_hdr->merkle_root[0], blk_hdr->merkle_root[1], blk_hdr->merkle_root[2], blk_hdr->merkle_root[3],
		    blk_hdr->timestamp, blk_hdr->bits, blk_hdr->nonce);
		if (i > 0) {
			bool is_correct = memcmp(hash, blk_hdr->prev_hash, sizeof(hash)) == 0;
			if (!is_correct) {
				all_correct = false;
				syslog(LOG_ERR, "%s: hash of block header %zu is incorrect", __func__, i - 1);
			}
		}
		calc_block_hdr_hash(blk_hdr, hash);
		const size_t fetch_block_interval = 100;
		if ((i % fetch_block_interval) == 0) {
			syslog(LOG_DEBUG, "%s: requesting full block #%zu", __func__, i);
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
	char hash_str[HASH_LEN * 2 + 1];
	uint8_t merkle_root[HASH_LEN];

	(void)peer;

	calc_block_hdr_hash(&block->hdr, hash);
	calc_merkle_root(block, merkle_root);

	syslog(LOG_DEBUG, "%s: block %02x%02x%02x%02x..., %lu txns", __func__, hash[0], hash[1], hash[2], hash[3], block->hdr.tx_count);
	syslog(LOG_DEBUG, "%s:       Merkle tree root: %s", __func__, strhash(block->hdr.merkle_root, hash_str));
	syslog(LOG_DEBUG, "%s        calculated Merkle tree root: %s", __func__, strhash(merkle_root, hash_str));

	if (memcmp(block->hdr.merkle_root, merkle_root, HASH_LEN) != 0) {
		syslog(LOG_ERR, "%s:       merkle root mismatch, is %02x%02x%02x%02x..., should be %02x%02x%02x%02x...",
		    __func__,
		    block->hdr.merkle_root[0], block->hdr.merkle_root[1], block->hdr.merkle_root[2], block->hdr.merkle_root[3],
		    merkle_root[0], merkle_root[1], merkle_root[2], merkle_root[3]);
	}

	for (size_t i = 0; i < block->hdr.tx_count; ++i) {
		struct tx_msg *tx = &block->tx[i];
		syslog(LOG_DEBUG, "%s:       tx #%zu: ver %u, %lu in, %lu out, locktime %u", __func__, i, tx->version, tx->in_count, tx->out_count, tx->locktime);
		for (size_t j = 0; j < tx->in_count; ++j) {
			uint8_t *out_hash = tx->in[j]->prev_output.hash;
			syslog(LOG_DEBUG, "%s:       tx #%zu: input #%zu from %02x%02x%02x%02x...", __func__, i, j, out_hash[0], out_hash[1], out_hash[2], out_hash[3]);
		}
		for (size_t j = 0; j < tx->out_count; ++j) {
			uint8_t *scr = tx->out[j]->pk_script;
			syslog(LOG_DEBUG, "%s:       tx #%zu: output #%zu, value %lu, script len %lu, script %02x%02x%02x%02x...", __func__, i, j, tx->out[j]->value, tx->out[j]->pk_script_len, scr[0], scr[1], scr[2], scr[3]);
		}
	}
	return 0;
}

static int
handle_message(struct peer *peer, union message *msg)
{
	int error = 0;
	switch (msg->type) {
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
	free_msg(msg);
	return error;
}

static void
epoll_add(int epfd, struct peer *peer)
{
	struct epoll_event ev;

	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.ptr = peer;
	int error = epoll_ctl(epfd, EPOLL_CTL_ADD, peer->proto.conn, &ev);
	if (error) {
		syslog(LOG_ERR, "failed to add peer's connection to epoll fd, errno %d", errno);
		exit(EXIT_FAILURE);
	}
}

static void
epoll_del(int epfd, struct peer *peer)
{
	struct epoll_event ev;

	memset(&ev, 0, sizeof(ev));
	int error = epoll_ctl(epfd, EPOLL_CTL_DEL, peer->proto.conn, &ev);
	if (error) {
		syslog(LOG_ERR, "failed to remove peer's connection from epoll fd, errno %d", errno);
		exit(EXIT_FAILURE);
	}
}

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	openlog("btcd", LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_USER);

	int epfd = epoll_create(1);
	if (epfd == -1) {
		syslog(LOG_ERR, "epoll_create failed, errno %d", errno);
		return EXIT_FAILURE;
	}

	srand(time(NULL) ^ getpid());

	struct peer *peers = discover_peers();
	for (struct peer *peer = peers; peer != NULL; peer = peer->next) {
		int error = peer_connect(peer);
		if (error) {
			continue;
		}

		epoll_add(epfd, peer);

		for (;;) {
			struct epoll_event ev;
			int rc = epoll_wait(epfd, &ev, 1, -1);
			switch (rc) {
			case -1:
				syslog(LOG_ERR, "epoll_wait failed, errno %d", errno);
				exit(EXIT_FAILURE);
			case 0:
				continue;
			}

			union message *msg = NULL;
			error = peer_read_message(peer, &msg);
			if (error) {
				syslog(LOG_ERR, "%s: failed to read a message from peer, return code %d", __func__, error);
				free_msg(msg);
				epoll_del(epfd, peer);
				close(peer->proto.conn);
				break;
			}

			if (msg != NULL)
				handle_message(peer, msg);
		}
	}

	return EXIT_SUCCESS;
}
