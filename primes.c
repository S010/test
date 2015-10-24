#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>

static bool
is_prime(unsigned long i)
{
	unsigned long	 j;
	unsigned long	 root;

	root = sqrt(i);

	for (j = 3; j <= root; j++)
		if (i % j == 0)
			return false;
	return true;
}

static void
primes(unsigned long i, unsigned long n)
{
	unsigned long	 count;

	if (i % 2 == 0)
		i++;

	while (n == 0 || count < n) {
		if (is_prime(i)) {
			printf("%lu\n", i);
			count++;
		}
		i += 2;
	}
}

/*
 * Print prime numbers.
 */
int
main(int argc, char **argv)
{
	unsigned long	 start = 2;
	unsigned long	 n = 0;

	if (argc > 1)
		start = strtoul(argv[1], NULL, 10);
	if (argc > 2)
		n = strtoul(argv[2], NULL, 10);

	primes(start, n);

	return 0;
}
