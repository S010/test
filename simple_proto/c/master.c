#include <sys/types.h>
#include <sys/uio.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include "msg.h"

static ssize_t
xwrite(int fd, const void *buf, size_t bufsize)
{
	ssize_t	 n;

	n = write(fd, buf, bufsize);
	if (n == -1)
		err(EXIT_FAILURE, "write");
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
	} write_tab[] = {
		{ &msg.type, sizeof msg.type },
		{ &msg.len, sizeof msg.len },
		{ msg.data, 0, &msg.len },
		{ &msg.chksum, sizeof msg.chksum },
		{ NULL },
	}, *p;
	int		 i;
	int		 j;
	ssize_t		 nwrite;

	for (i = 0; i < 0x100; ++i) {
		msg.type = i;
		msg.len = i;
		for (j = 0; j < i; ++j)
			msg.data[j] = i;
		msg.chksum = 0;
		msg.chksum = msg_chksum(msg.chksum, &msg.type, sizeof msg.type);
		msg.chksum = msg_chksum(msg.chksum, &msg.len, sizeof msg.len);
		msg.chksum = msg_chksum(msg.chksum, msg.data, msg.len);

		for (p = write_tab; p->ptr != NULL; ++p) {
			nwrite = xwrite(STDOUT_FILENO, p->ptr, p->size ? p->size : *p->psize);
			if (nwrite != (p->size ? p->size : *p->psize))
				errx(EXIT_FAILURE, "short write");
		}
	}

	return EXIT_SUCCESS;
}
