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
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

static void
usage(void)
{
	puts("usage: bitzip <file>");
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

/* load file */
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

static size_t
zip(const unsigned char *buf, size_t size, unsigned char *outbuf)
{
	const size_t	 nbits = size * CHAR_BIT;
	size_t		 readp;
	size_t		 writep = 0;
	size_t		 len;
	size_t		 runlen;
	unsigned char	 bit;

#define BIT_AT(i) (buf[(i) / CHAR_BIT] & (1 << ((i) % CHAR_BIT)))
#define PUT_BIT(b)										\
	do {											\
		if (outbuf != NULL)								\
			outbuf[writep / CHAR_BIT] |= (b ? 1 : 0) << (writep % CHAR_BIT);	\
		++writep;									\
	} while (0)

	for (readp = 0; readp < nbits; /*empty*/) {
		bit = BIT_AT(readp);
		len = 1;
		while (readp + len < nbits) {
			if ((bit && BIT_AT(readp + len)) || (!bit && !BIT_AT(readp + len)))
				++len;
			else
				break;
		}
		readp += len--;
		do {
			PUT_BIT(bit);
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

#undef BIT_AT
#undef PUT_BIT
}

int
main(int argc, char **argv)
{
	unsigned char	*buf;
	size_t		 size;

	if (argc < 2) {
		usage();
		return EXIT_FAILURE;
	}
	load(argv[1], &buf, &size);
	printf("\n%d bits\n", (int) zip(buf, size, NULL));
	free(buf);

	return EXIT_SUCCESS;
}
