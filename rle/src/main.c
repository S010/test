/*
 * Copyright (c) 2014 Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
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

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include "codec.h"

static int	 dflag;
static int	 vflag;

static void
usage(const char *progname)
{
	printf("usage: %s [-d] <infile> <outfile>\n"
	    "  -d -- decode\n"
	    "  -v -- be verbose", progname);
}

void *
xmalloc(size_t size)
{
	void	*p;

	p = malloc(size);
	if (p == NULL)
		err(EXIT_FAILURE, "malloc");
	return p;
}

ssize_t
xwrite(int fd, const void *buf, size_t size)
{
	ssize_t	 n;

	n = write(fd, buf, size);
	if (n == -1)
		err(1, "write");
	else if (n < size)
		errx(1, "short write");
	return n;
}

void
load(const char *path, unsigned char **bufp, size_t *sizep)
{
	struct stat	 st;
	int		 fd;
	ssize_t		 nread;
	void		*buf;

	if (stat(path, &st) == -1)
		err(EXIT_FAILURE, "stat");
	fd = open(path, O_RDONLY);
	if (fd == -1)
		err(EXIT_FAILURE, "open %s", path);
	buf = xmalloc(st.st_size);
	nread = read(fd, buf, st.st_size);
	if (nread == -1)
		err(1, "read");
	*bufp = buf;
	*sizep = st.st_size;
}

void
store(const char *path, const void *buf, size_t size, int flags)
{
	int			 fd;

	fd = open(path, O_WRONLY | flags, S_IRUSR | S_IWUSR);
	if (fd == -1)
		err(1, "open %s", path);
	xwrite(fd, buf, size);
	close(fd);
}

static struct timeval
stopwatch(void)
{
	static struct timeval	 t1;
	static struct timeval	 t2;
	struct timeval		 dt;
	static enum {
		STOPPED,
		RUNNING,
		N_STATES
	} state = STOPPED;

	if (state == STOPPED) {
		if (gettimeofday(&t1, NULL) < 0)
			err(EXIT_FAILURE, "gettimeofday");
	} else {
		if (gettimeofday(&t2, NULL) < 0)
			err(EXIT_FAILURE, "gettimeofday");
		dt.tv_sec = t2.tv_sec - t1.tv_sec;
		dt.tv_usec = t2.tv_usec - t1.tv_usec;
		if (dt.tv_usec < 0) {
			--dt.tv_sec;
			dt.tv_usec += 1000000;
		}
	}
	state = (state + 1) % N_STATES;
	return dt;
}

int
main(int argc, char **argv)
{
	int		 ch;
	extern int	 optind;
	struct timeval	 dt;

	while ((ch = getopt(argc, argv, "dvh")) != -1) {
		switch (ch) {
		case 'd':
			++dflag;
			break;
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;
		case 'v':
			++vflag;
			break;
		default:
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}
	argc -= optind;
	argv += optind;
	if (argc < 2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	(void) stopwatch();
	if (dflag)
		decode(argv[0], argv[1]);
	else
		encode(argv[0], argv[1]);
	dt = stopwatch();
	if (vflag)
		printf("%ld.%06ld\n", (long) dt.tv_sec, (long) dt.tv_usec);

	return EXIT_SUCCESS;
}

