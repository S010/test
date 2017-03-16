#ifndef PEER_H
#define PEER_H

#include <netinet/in.h>

#include "protocol.h"

/*
 * A peer node in the Bitcoin P2P network.
 */
struct peer {
	enum peer_state {
		PEER_DISCONNECTED,
		PEER_VERSION,
		PEER_VERACK,
		PEER_CONNECTED
	} state;
	int addr_family;
	struct sockaddr_in6 addr;
	struct version_msg version;
	struct protocol proto;
	struct peer *next;
};

struct peer *discover_peers(void);
void print_peers(struct peer *peer);
int peer_connect(struct peer *peer);
void *peer_free_list(struct peer *peer);

#endif
