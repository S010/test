#include <stdio.h>
#include <stdlib.h>

int
sum(int x)
{
	return x + sum(x);
}

int
main(int argc, char **argv)
{
	printf("%d\n", sum(argc));

	return EXIT_SUCCESS;
}
