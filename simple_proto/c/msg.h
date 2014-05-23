#ifndef MSG_H
#define MSG_H

#include <stdint.h>
#include <stddef.h>

struct msg {
	uint8_t	 type;
	uint8_t	 len;
	uint8_t	 data[0xff];
	uint8_t	 chksum;
};

uint8_t msg_chksum(uint8_t chksum, const void *buf, size_t bufsize);

#endif
