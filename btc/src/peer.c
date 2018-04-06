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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdbool.h>

#include "protocol.h"
#include "peer.h"
#include "xmalloc.h"
#include "util.h"

static const char self_user_agent[] = "/btcd:0.0.1/";

static void
print_peers(struct peer *peer)
{
	void *addr;
	char addrstr[INET6_ADDRSTRLEN];
	while (peer != NULL) {
		switch (peer->addr_family) {
		case AF_INET:
			addr = peer->addr.sin6_addr.s6_addr + 12;
			break;
		case AF_INET6:
			addr = &peer->addr.sin6_addr;
			break;
		default:
			syslog(LOG_DEBUG, "(unknown address family)");
			continue;
		}
		syslog(LOG_DEBUG, "%s\n", inet_ntop(peer->addr_family, addr, addrstr, sizeof(addrstr)));
		peer = peer->next;
	}
}

struct peer *
discover_peers(void)
{
	const char *domain_name = "seed.bitcoin.sipa.be.";
	struct addrinfo *addrs = NULL;
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(domain_name, NULL, &hints, &addrs) == -1) {
		syslog(LOG_ERR, "%s: getaddrinfo failed, errno %d", __func__, errno);
		return NULL;
	}

	struct peer *head, *tail, *peer;
	head = tail = peer = NULL;
	for (struct addrinfo *ai = addrs; ai != NULL; ai = ai->ai_next) {
		switch (ai->ai_family) {
		case AF_INET:
		case AF_INET6:
			peer = xcalloc(1, sizeof(*peer));
			peer->proto.buf = xcalloc(1, sizeof(*peer->proto.buf));
			peer->addr_family = ai->ai_family;
			if (ai->ai_family == AF_INET6) {
				peer->addr = *((struct sockaddr_in6 *)ai->ai_addr);
			} else {
				uint8_t *s6_addr_ptr = peer->addr.sin6_addr.s6_addr;
				void *s_addr_ptr = &((struct sockaddr_in *)ai->ai_addr)->sin_addr.s_addr;
				memset(s6_addr_ptr + 10, 0xff, 2);
				memcpy(s6_addr_ptr + 12, s_addr_ptr, 4);
			}
			if (head == NULL) {
				head = tail = peer;
			} else {
				tail->next = peer;
				tail = peer;
			}
			break;
		default:
			continue;
		}

	}
	freeaddrinfo(addrs);

	if (addrs == NULL) {
		syslog(LOG_DEBUG, "%s: no peers discovered, perhaps there's no internet connection", __func__);
	} else {
		syslog(LOG_DEBUG, "%s: discovered:", __func__);
		print_peers(head);
	}

	return head;
}

int
peer_send_version(struct peer *peer)
{
	struct version_msg msg;
	const struct in6_addr loopback = IN6ADDR_LOOPBACK_INIT;
	int error;

	memset(&msg, 0, sizeof(msg));

	msg.version = PROTOCOL_VERSION;
	msg.services = 0;
	msg.timestamp = time(NULL);

	msg.recv_addr.services = 0;
	msg.recv_addr.ip = peer->addr.sin6_addr;
	msg.recv_addr.port = ntohs(peer->addr.sin6_port);

	msg.trans_addr.services = 0;
	msg.trans_addr.ip = loopback;
	msg.trans_addr.port = ntohs(peer->addr.sin6_port);

	msg.nonce = 0;
	msg.user_agent_len = sizeof(self_user_agent) - 1;
	strcpy(msg.user_agent, self_user_agent);
	msg.start_height = 0;
	msg.relay = true;

	error = write_version_msg(&msg, peer->proto.conn);
	if (error) {
		syslog(LOG_ERR, "%s: failed to write version message to peer", __func__);
		return error;
	}

	// FIXME Set the states of peer and protocol.

	return 0;
}

int
peer_connect(struct peer *peer)
{
	int family;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
	struct sockaddr *sa;
	socklen_t sa_len;
	int conn;
	int error;

	family = peer->addr_family;

	memset(&sin, 0, sizeof(sin));
	memset(&sin6, 0, sizeof(sin6));

	switch (family) {
	case AF_INET:
		sin.sin_family = AF_INET;
		sin.sin_port = peer->addr.sin6_port = htons(MAINNET_PORT);
		memcpy(&sin.sin_addr.s_addr, peer->addr.sin6_addr.s6_addr + 12, 4);
		sa = (struct sockaddr *)&sin;
		sa_len = sizeof(sin);
		break;
	case AF_INET6:
		sin6 = peer->addr;
		sin6.sin6_port = peer->addr.sin6_port = htons(MAINNET_PORT);
		sa = (struct sockaddr *)&sin6;
		sa_len = sizeof(sin6);
		break;
	default:
		syslog(LOG_ERR, "%s: unknown address family %d", __func__, family);
		return -1;
	}

	conn = socket(family, SOCK_STREAM, 0);
	if (conn == -1) {
		syslog(LOG_WARNING, "%s: failed to create stream socket for address family %d, errno %d",
		    __func__, family, errno);
		return errno;
	}


	char addrstr[INET6_ADDRSTRLEN];
	inet_ntop(family, family == AF_INET ? (void *)&sin.sin_addr : (void *)&sin6.sin6_addr,
	    addrstr, sizeof(addrstr));

	syslog(LOG_DEBUG, "%s: connecting to %s", __func__, addrstr);
	if (connect(conn, sa, sa_len) == -1) {
		syslog(LOG_WARNING, "%s: failed to connect to %s", __func__, addrstr);
		close(conn);
		return -1;
	}

	peer->proto.conn = conn;

	error = peer_send_version(peer);
	if (error) {
		syslog(LOG_ERR, "%s: failed to send version message to peer", __func__);
		return error;
	}

	// FIXME Have state aware functions take necessary actions.
	error = read_version_msg(peer->proto.conn, &peer->version);
	if (error) {
		syslog(LOG_ERR, "%s: failed to read version message from peer", __func__);
		return error;
	}

	error = write_verack_msg(peer->proto.conn);
	if (error) {
		syslog(LOG_ERR, "%s: failed to send verack message to peer", __func__);
		return error;
	}

	error = read_verack_msg(peer->proto.conn);
	if (error) {
		syslog(LOG_ERR, "%s: failed to read verack message from peer", __func__);
		return error;
	}

	peer->state = PEER_CONNECTED;
	peer->proto.state = WAIT_HEADER;

	if (fcntl(conn, F_SETFL, O_NONBLOCK) == -1) {
		syslog(LOG_CRIT, "%s: failed to set non-blocing mode for peer's socket, errno %d", __func__, errno);
		exit(EXIT_FAILURE);
	}

	return 0;
}

void *
peer_free_list(struct peer *peer)
{
	struct peer *next;
	while (peer != NULL) {
		next = peer->next;
		if (peer->proto.conn != -1)
			close(peer->proto.conn);
		free(peer);
		peer = next;
	}
	return NULL;
}

static struct buffer *
grow_buffer(struct buffer *buf, size_t amount)
{
	buf = xrealloc(buf, sizeof(*buf) + buf->size + amount);
	buf->size += amount;
	return buf;
}

int
peer_recv_data(struct peer *peer)
{
	const size_t grow_size = 8192;
	struct buffer *buf;
	ssize_t n_read;
	int error = 0;

	if (peer->proto.state == HAVE_MESSAGE) {
		return 0;
	}

	buf = peer->proto.buf;
	do {
		if (buf->len == buf->size)
			buf = peer->proto.buf = grow_buffer(buf, grow_size);
		n_read = read(peer->proto.conn, buf->bytes + buf->len, buf->size - buf->len);
		if (n_read > 0) {
			buf->len += n_read;
			syslog(LOG_DEBUG, "%s: read %d bytes from peer", __func__, (int)n_read);
		}
	} while (buf->len == buf->size);

	if (n_read == -1 && errno != EAGAIN) {
		syslog(LOG_ERR, "failed to read from peer, errno %d", errno);
		return -1;
	}

	switch (peer->proto.state) {
	case WAIT_HEADER:
		if (buf->len >= MSG_HDR_LEN) {
			// FIXME Handle unmarshal error by reconnecting to peer
			size_t size = unmarshal_msg_hdr(buf->bytes, &peer->proto.hdr);
			syslog(LOG_DEBUG, "%s: %ssuccessfully unmarshalled header", __func__, size == MSG_HDR_LEN ? "" : "un");
			peer->proto.state = WAIT_MESSAGE;
		}
		break;
	case WAIT_MESSAGE:
		if (buf->len >= MSG_HDR_LEN + peer->proto.hdr.payload_size) {
			// FIXME Handle unmarshal error by reconnecting to peer
			// FIXME Implement a generic message unmarshalling function
			error = unmarshal_msg(buf, &peer->proto.hdr, &peer->proto.msg);
			syslog(LOG_DEBUG, "%s: %ssuccessfully unmarshalled a message", __func__, error ? "un" : "");
			size_t next_len = buf->len - (MSG_HDR_LEN + peer->proto.hdr.payload_size);
			memmove(buf->bytes, buf->bytes + buf->len - next_len, next_len);
			buf->len = next_len;
			// FIXME Make the buffer smaller?
			peer->proto.state = HAVE_MESSAGE;
		}
		break;
	default:
		break;
	}

	return error;
}

int
peer_read_message(struct peer *peer, union message **msg)
{
	int error = 0;

	switch (peer->proto.state) {
	case WAIT_HEADER:
	case WAIT_MESSAGE:
		error = peer_recv_data(peer);
		break;
	case HAVE_MESSAGE:
		break;
	}

	// FIXME Handle peer_recv_data() error.

	if (peer->proto.state == HAVE_MESSAGE) {
		*msg = peer->proto.msg;
		peer->proto.msg = NULL;
		peer->proto.state = WAIT_HEADER;
		error = peer_recv_data(peer);
	}

	return error;
}
