#ifndef MSG_H
#define MSG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MSG_N_SIZE_BITS		10

// Return msg type from hdr.
#define MSG_TYPE(hdr)		((hdr) >> MSG_N_SIZE_BITS)
// Return msg size from hdr.
#define MSG_SIZE(hdr)		((hdr) & ~(~0 << MSG_N_SIZE_BITS))

// Compose msg hdr.
#define MSG_HDR(type, size)	(((type) << MSG_N_SIZE_BITS) | ((size) & ~(~0 << MSG_N_SIZE_BITS)))

struct msg {
	uint16_t	 hdr;
	uint8_t		 data[~(~0 << MSG_N_SIZE_BITS)];
	uint8_t		 chksum;
};

// Return true if chksum is correct.
uint8_t msg_chksum(const struct msg *);

#endif

