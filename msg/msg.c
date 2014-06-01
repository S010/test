#include "msg.h"

static uint8_t
chksum(uint8_t chksum, const void *buf, size_t size)
{
	const uint8_t	*p = buf;

	while (size--) {
		chksum = (chksum & 1 << 7) | (chksum >> 1);
		chksum ^= *p++;
	}
	return chksum;
}

uint8_t
msg_chksum(const struct msg *msg)
{
	uint8_t		 c = 0;

	c = chksum(c, &msg->hdr, sizeof(msg->hdr));
	c = chksum(c, msg->data, MSG_SIZE(msg->hdr));

	return c;
}

