#include <sys/types.h>
#include <sys/uio.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "msg.h"

static ssize_t
xwrite(int fd, const void *buf, size_t size)
{
	ssize_t	 n;

	n = write(fd, buf, size);
	if (n == -1)
		err(EXIT_FAILURE, "write");
	else if (n < size)
		err(EXIT_FAILURE, "write");
	return n;
}

int
main(int argc, char **argv)
{
	struct msg	 msg;
	int		 i;

	for (i = 0; i < 32; ++i) {
		msg.hdr = MSG_HDR(i, i);
		memset(msg.data, i, i);
		msg.chksum = msg_chksum(&msg);

		xwrite(STDOUT_FILENO, &msg.hdr, sizeof msg.hdr);
		xwrite(STDOUT_FILENO, msg.data, MSG_SIZE(msg.hdr));
		xwrite(STDOUT_FILENO, &msg.chksum, sizeof msg.chksum);
	}

	return EXIT_SUCCESS;
}
