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
		syslog(LOG_DEBUG, "%s: inv_vec: type %d, %02x%02x%02x%02x...", __func__, msg->inv.inv_vec[i].type,  p[0], p[1], p[2], p[3]);
	}
	return 0;
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
	default:
		break;
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
			free(msg);
			msg = NULL;
		}

		break;
	}

	return 0;
}
