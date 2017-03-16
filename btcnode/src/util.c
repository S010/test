#include <stdio.h>
#include <stdint.h>

#include "util.h"

void
hexdump(const void *ptr, size_t size)
{
	const uint8_t *buf = ptr;
	size_t i;
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
	if ((i % 16) == 0)
		putchar('\n');
}
