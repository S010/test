#include <unistd.h>
#include <err.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void dump(char *p, long size);

int
main(int argc, char **argv)
{
	char *p;
	long size;

	while (++argv, --argc) {
		size = atol(*argv);
		p = malloc(size);
		if (p == NULL) {
			warn("malloc");
			continue;
		}
		dump(p, size);
		free(p);
	}

	return 0;
}

static void
dump(char *p, long size)
{
	char *end;

	printf("p=%p\n", p);
	for (end = p + size; p < end; ++p)
		printf("%x", *p);
	putchar('\n');
}
