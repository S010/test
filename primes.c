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
primes_nocache(unsigned long i, unsigned long n)
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

static bool
is_prime_cached(unsigned long i, unsigned long *arr, unsigned long count)
{
	unsigned long	 root;

	root = sqrt(i);

	arr++, count--; /* skip 2 */

	while (*arr <= root && count > 0) {
		if (i % *arr == 0)
			return false;
		arr++;
		count--;
	}

	return true;
}

static void
primes_cached(unsigned long i, unsigned long n)
{
	unsigned long	*arr = NULL; /* Array of prime numbers */
	unsigned long	 count = 1;

	arr = reallocarray(arr, count, sizeof i);
	arr[count - 1] = 2;

	i = 3;

	while (n == 0 || count < n) {
		if (is_prime_cached(i, arr, count)) {
			printf("%lu\n", i);
			arr = reallocarray(arr, ++count, sizeof i);
			arr[count - 1] = i;
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

	primes_cached(start, n);

	return 0;
}
