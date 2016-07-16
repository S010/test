#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME "\x03www\x03ibm\x02co\x02uk"

#define N 100000000

#define PRIME 2027
#define NOT_PRIME (67 * 71)
#define A_NUMBER NOT_PRIME

int
name_len1(const unsigned char *s)
{
	int len = 0;
	unsigned c;

	while (*s) {
		c = *s;
		if (c > 63) {
			fprintf(stderr, "error: %s: invalid name @%p, *s=%x\n", __func__, s, *s);
			return -1;
		}
		len += c;
		s += c + 1;
	}
	return len;
}

int
name_len2(const unsigned char *s)
{
	int len = 0;
	unsigned c;

	while (*s) {
		c = *s;
		if (c < 64) {
			len += c;
			s += c + 1;
		} else {
			fprintf(stderr, "error: %s: invalid name @%p, *s=%x\n", __func__, s, *s);
			return -1;
		}
	}
	return len;
}

int
is_prime(int n)
{
	for (int i = 2; i < n; i++) {
		if (n % i == 0)
			return 0;
	}
	return 1;
}

int
main(int argc, char **argv)
{
	unsigned count = 0;

	if (argc < 2)
		return 1;

	if (!strcmp(argv[1], "1")) {
		for (int i = 0; i < N; i++) {
			count += is_prime(A_NUMBER);
			count += name_len1((unsigned const char *)NAME);
		}
	} else {
		for (int i = 0; i < N; i++) {
			count += is_prime(A_NUMBER);
			count += name_len2((unsigned const char *)NAME);
		}
	}
	count += argc;

	printf("%u\n", count);

	return 0;
}

