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
xread(int fd, void *buf, size_t size)
{
	ssize_t	 n;

	n = read(fd, buf, size);
	if (n == -1)
		err(EXIT_FAILURE, "read");
	else if (n < size)
		errx(EXIT_FAILURE, "short read");
	return n;
}

int
main(int argc, char **argv)
{
	struct msg	 msg;

	for (;;) {
		xread(STDIN_FILENO, &msg.hdr, sizeof msg.hdr);
		xread(STDIN_FILENO, msg.data, MSG_SIZE(msg.hdr));
		xread(STDIN_FILENO, &msg.chksum, sizeof msg.chksum);

		if (msg_chksum(&msg) != msg.chksum)
			errx(EXIT_FAILURE, "msg chksum is incorrect");

		fprintf(stderr, "Msg: type=0x%02x len=%3d chksum=0x%02x\n", MSG_TYPE(msg.hdr), MSG_SIZE(msg.hdr), msg.chksum);
	}

	return EXIT_SUCCESS;
}
