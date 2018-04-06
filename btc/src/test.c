#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Returns the logarithm base two of i.
 * i must be an even number or zero.
 */
/*
unsigned
log_base_two(unsigned i)
{
	if (i == 0)
		return 0;

	if ((i & 1) == 0)
		i -= 1;

	unsigned count = 0;
	while (i != 0) {
		count += i & 1;
		i >>= 1;
	}
	return count;


}
*/

unsigned
log_base_two(unsigned i)
{
	unsigned result;

	i &= ~1u;

#define Y(x)	case (1u << x): i = x; break;
	switch (i) {
	 Y(1);  Y(2);  Y(3);  Y(4);  Y(5);  Y(6);  Y(7);  Y(8);
	 Y(9); Y(10); Y(11); Y(12); Y(13); Y(14); Y(15); Y(16);
	Y(17); Y(18); Y(19); Y(20); Y(21); Y(22); Y(23); Y(24);
	Y(25); Y(26); Y(27); Y(28); Y(29); Y(30); Y(31);
	default:
		i = 0;
	}
#undef Y

	return i;
}

int
main(int argc, char **argv)
{
	unsigned i = 4; 
	unsigned l;

	if (argc > 1)
		sscanf(argv[1], "%u", &i);

	l = log_base_two(i);
	printf("%u\n", l);

	return 0;
}
