#include <sys/types.h>
#include <sys/uio.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <err.h>

#include "msg.h"

static ssize_t
xread(int fd, void *buf, size_t bufsize)
{
	ssize_t	 n;

	n = read(fd, buf, bufsize);
	if (n == -1)
		err(1, "read");
	return n;
}

int
main(int argc, char **argv)
{
	struct msg	 msg;
	struct {
		void	*ptr;
		uint8_t	 size;
		uint8_t	*psize;
	} read_tab[] = {
		{ &msg.type,   sizeof msg.type             },
		{ &msg.len,    sizeof msg.len              },
		{ msg.data,    0,                 &msg.len },
		{ &msg.chksum, sizeof msg.chksum           },
		{ NULL },
	}, *p;
	ssize_t		 nread;
	uint8_t		 chksum;
	uint8_t		 size;

	for (;;) {
		chksum = 0;
		for (p = read_tab; p->ptr != NULL; ++p) {
			size = p->size ? p->size : *p->psize;
			nread = xread(STDIN_FILENO, p->ptr, size);
			if (nread != size)
				errx(EXIT_FAILURE, "short read");
			if (p->ptr != &msg.chksum)
				chksum = msg_chksum(chksum, p->ptr, size);
		}
		if (chksum != msg.chksum)
			errx(EXIT_FAILURE, "chksum mismatch: 0x%02x != 0x%02x", msg.chksum, chksum);

		fprintf(stderr, "Msg: type=0x%02x len=%3d chksum=0x%02x\n", msg.type, msg.len, msg.chksum);
	}

	return EXIT_SUCCESS;
}
