#include <stdio.h>

void
hello1(int argc, char **argv)
{
	if (argc < 2)
		return;

	printf("Hello, %s!\n", argv[1]);
}

int
hello2(int argc, char **argv)
{
	if (argc > 1) {
		printf("Hello, %s!\n", argv[1]);
	}
	return 0;
}

int
main(int argc, char **argv)
{
	hello1(argc, argv);
	hello2(argc, argv);

	return 0;
}
