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

#define _XOPEN_SOURCE 600

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

typedef size_t runlen_t;

static int	 dflag;
static int	 vflag;

static void
usage(void)
{
	puts("usage: bitrle [-d] <infile> <outfile>\n"
	    "  -d -- decode\n"
	    "  -v -- be verbose");
}

static void *
xmalloc(size_t size)
{
	void	*p;

	p = malloc(size);
	if (p == NULL)
		err(EXIT_FAILURE, "malloc");
	return p;
}

static ssize_t
xwrite(int fd, void *buf, size_t size)
{
	ssize_t	 n;

	n = write(fd, buf, size);
	if (n == -1)
		err(1, "write");
	else if (n < size)
		errx(1, "short write");
	return n;
}

static void
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

static void
store(const char *path, unsigned char *buf, size_t size, size_t nbits)
{
	int	 fd;

	fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd == -1)
		err(1, "open %s", path);
	if (nbits > 0)
		xwrite(fd, &nbits, sizeof nbits);
	xwrite(fd, buf, size);
	close(fd);
}


#define BIT_AT(i) (buf[(i) / CHAR_BIT] & (1 << ((i) % CHAR_BIT)))
#define PUT_BIT(b)										\
	do {											\
		if (outbuf != NULL) {								\
			if (b)									\
				outbuf[writep / CHAR_BIT] |= (1 << (writep % CHAR_BIT));	\
			else									\
				outbuf[writep / CHAR_BIT] &= (~(1 << (writep % CHAR_BIT)));	\
		}										\
		++writep;									\
	} while (0)

static size_t
bitrle_encode(const unsigned char *buf, size_t size, unsigned char *outbuf)
{
	const size_t	 nbits = size * CHAR_BIT;
	size_t		 readp;
	size_t		 writep = 0;
	size_t		 len;
	runlen_t	 runlen;
	unsigned char	 bit;

	for (readp = 0; readp < nbits; /*empty*/) {
		bit = BIT_AT(readp);
		len = 1;
		while (readp + len < nbits) {
			if ((bit && BIT_AT(readp + len)) || (!bit && !BIT_AT(readp + len)))
				++len;
			else
				break;
		}
		readp += len;
		do {
			PUT_BIT(bit);
			--len;
			runlen = ~0;
			while (runlen > len)
				runlen >>= 1;
			len -= runlen;
			while (runlen > 0) {
				PUT_BIT(1);
				runlen >>= 1;
			}
			PUT_BIT(0);
		} while (len > 0);
	}

	return writep;

}

static size_t
bitrle_decode(const unsigned char *buf, size_t nbits, unsigned char *outbuf)
{
	size_t		 readp;
	size_t		 writep = 0;
	unsigned char	 bit;
	runlen_t	 runlen;

	for (readp = 0; readp < nbits; ++readp) {
		bit = BIT_AT(readp);
		++readp;
		for (runlen = 0; BIT_AT(readp); ++readp) {
			runlen <<= 1;
			runlen |= 1;
		}
		PUT_BIT(bit);
		while (runlen--)
			PUT_BIT(bit);
	}

	return writep;
}

#undef BIT_AT
#undef PUT_BIT

static struct timespec
timediff(struct timespec *t1, struct timespec *t2)
{
	struct timespec	 dt;

	dt = *t2;
	if (dt.tv_nsec < t1->tv_nsec) {
		--dt.tv_sec;
		dt.tv_nsec += 1000000000;
	}
	dt.tv_sec -= t1->tv_sec;
	dt.tv_nsec -= t1->tv_nsec;

	return dt;
}

static void
encode(const char *inpath, const char *outpath)
{
	struct timespec	 t1;
	struct timespec	 t2;
	struct timespec	 dt;
	unsigned char	*inbuf;
	size_t		 insize;
	unsigned char	*outbuf;
	size_t		 outsize;
	size_t		 nbits;

	load(inpath, &inbuf, &insize);
	nbits = bitrle_encode(inbuf, insize, NULL);
	outsize = nbits / 8 + ((nbits % 8) ? 1 : 0);
	outbuf = xmalloc(outsize);
	if (clock_gettime(CLOCK_MONOTONIC, &t1) == -1)
		err(EXIT_FAILURE, "clock_gettime");
	bitrle_encode(inbuf, insize, outbuf);
	if (clock_gettime(CLOCK_MONOTONIC, &t2) == -1)
		err(EXIT_FAILURE, "clock_gettime");
	dt = timediff(&t1, &t2);
	if (vflag)
		printf("encoded in %lds %ldns\n", (long) dt.tv_sec, (long) dt.tv_nsec);
	store(outpath, outbuf, outsize, nbits);
	free(inbuf);
	free(outbuf);
}

static void
decode(const char *inpath, const char *outpath)
{
	struct timespec	 t1;
	struct timespec	 t2;
	struct timespec	 dt;
	unsigned char	*inbuf;
	size_t		 insize;
	unsigned char	*outbuf;
	size_t		 outsize;
	size_t		 nbitsin;
	size_t		 nbitsout;
	size_t		 i;

	load(inpath, &inbuf, &insize);
	for (i = 0, nbitsin = 0; i < sizeof(nbitsin); ++i)
		nbitsin |= inbuf[i] << (CHAR_BIT * i);
	nbitsout = bitrle_decode(inbuf + sizeof(nbitsin), nbitsin, NULL);
	outsize = nbitsout / CHAR_BIT + ((nbitsout % CHAR_BIT) ? 1 : 0);
	outbuf = xmalloc(outsize);
	if (clock_gettime(CLOCK_MONOTONIC, &t1) == -1)
		err(EXIT_FAILURE, "clock_gettime");
	bitrle_decode(inbuf + sizeof(nbitsin), nbitsin, outbuf);
	if (clock_gettime(CLOCK_MONOTONIC, &t2) == -1)
		err(EXIT_FAILURE, "clock_gettime");
	dt = timediff(&t1, &t2);
	if (vflag)
		printf("decoded in %lds %ldns\n", (long) dt.tv_sec, (long) dt.tv_nsec);
	store(outpath, outbuf, outsize, 0);
	free(inbuf);
	free(outbuf);
}

int
main(int argc, char **argv)
{
	int		 ch;
	extern int	 optind;

	while ((ch = getopt(argc, argv, "dvh")) != -1) {
		switch (ch) {
		case 'd':
			++dflag;
			break;
		case 'h':
			usage();
			return EXIT_SUCCESS;
		case 'v':
			++vflag;
			break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}
	argc -= optind;
	argv += optind;
	if (argc < 2) {
		usage();
		return EXIT_FAILURE;
	}

	if (dflag)
		decode(argv[0], argv[1]);
	else
		encode(argv[0], argv[1]);

	return EXIT_SUCCESS;
}

