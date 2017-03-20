#include <stdio.h>
#include <stdint.h>

#include "util.h"

void
hexdump(const void *ptr, size_t size)
{
	const uint8_t *buf = ptr;
	size_t i;
	fflush(stdout);
	for (i = 0; i < size; ++i) {
		if (i > 0 && (i % 16) == 0)
			putchar('\n');
		else if (i > 0 && (i % 8) == 0)
			printf("  ");
		else if (i > 0 && (i % 4) == 0)
			putchar(' ');
		if (i == 0 || (i % 16) == 0)
			printf("0x%08lx| ", i);
		printf("%02x", buf[i]);
	}
	if (((i - 1) % 16) != 0)
		putchar('\n');
	fflush(stdout);
}

char *
strhash(const uint8_t *hash /*[32]*/, char *out /*[64+1]*/)
{
	static char buf[64 + 1];
	char *result;

	if (out == NULL)
		out = buf;

	result = out;

	char hex[] = "0123456789abcdef";
	for (int i = 0; i < 32; ++i) {
		*out++ = hex[*hash >> 4];
		*out++ = hex[*hash & 0xf];
		hash++;
	}
	*out++ = '\0';

	return result;
}

