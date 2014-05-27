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

static void
zip(const unsigned char *buf, size_t size)
{
	const size_t	 numinbits = size * CHAR_BIT;
	size_t		 pos;
	size_t		 len;
	size_t		 runlen;
	unsigned char	 bit;

#define BIT_AT(pos) (buf[(pos) / 8] & (1 << ((pos) % CHAR_BIT)))

	for (pos = 0; pos < numinbits; /*empty*/) {
		bit = BIT_AT(pos); 
		len = 1;
		while (pos + len < numinbits) {
			if ((bit && BIT_AT(pos + len)) || (!bit && !BIT_AT(pos + len)))
				++len;
			else
				break;
		}
		//printf("%d %d: ", bit ? 1 : 0, (int) len);
		pos += len--;
		do {
			runlen = ~0;
			while (runlen > len)
				runlen >>= 1;
			len -= runlen;
			putchar(bit ? '1' : '0');
			while (runlen > 0) {
				putchar('1');
				runlen >>= 1;
			}
			putchar('0');
		} while (len > 0);
		putchar('\n');
		/*
		putchar(bit ? '1' : '0');
		while (len > 0) {
		}
		putchar('0');
		*/
	}

#undef BIT_AT
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
	zip(buf, size);
	free(buf);

	return EXIT_SUCCESS;
}
