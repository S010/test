#include "msg.h"

uint8_t
msg_chksum(uint8_t chksum, const void *buf, size_t bufsize)
{
	const uint8_t	*p = buf;

	while (bufsize--) {
		chksum = (chksum & 1 << 7) | (chksum >> 1);
		chksum ^= *p;
	}
	return chksum;
}

