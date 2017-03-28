#include <stdio.h>
#include <stdlib.h>

void
sort(int *a, int n)
{
	for (int i = n - 1; i > 0; --i) {
		int key = a[i];
		for (int j = i - 1; j >= 0 && a[j] > key; --j) {
			a[j+1] = a[j];
			a[j] = key;
		}
	}
}

void
print_array(int *a, int n)
{
	while (n--) {
		printf("%d ", *a++);
	}
	putchar('\n');
	fflush(stdout);
}

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	int a1[] = { 5, 2, 9 };
	int a2[] = { 8, 9 };
	int a3[] = { 1, 2, 3 };
	int a4[] = { 9, 9, 10, 4, 22, 23, 21 };
	int a5[] = { 1, 5, 3, 8 };

	#define TEST(a) \
		printf(#a " before: "); \
		print_array(a, sizeof(a)/sizeof(*a)); \
		sort(a, sizeof(a)/sizeof(*a)); \
		printf(#a " after: "); \
		print_array(a, sizeof(a)/sizeof(*a));

	TEST(a1);
	TEST(a2);
	TEST(a3);
	TEST(a4);
	TEST(a5);

	return 0;
}
