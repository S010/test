#include <stdio.h>
#include <stdlib.h>

void
partition(int *a, int left, int right, int *pivot)
{
	while (left < right) {
		if (a[left] > a[right]) {
			int tmp = a[left];
			a[left] = a[right];
			a[right] = tmp;
			--right;
		}
		++left;
	}
	*pivot = right;
}

void
sort(int *a, int left, int right)
{
	int pivot;
	partition(a, left, right, &pivot);
	if (left < pivot - 1)
		sort(a, left, pivot - 1);
	if (pivot < right)
		sort(a, pivot, right);
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
	int a5[] = { 1, 5, 3, 8, 9 };

	#define TEST(a) \
		printf(#a " before: "); \
		print_array(a, sizeof(a)/sizeof(*a)); \
		sort(a, 0, sizeof(a)/sizeof(*a) - 1); \
		printf(#a " after: "); \
		print_array(a, sizeof(a)/sizeof(*a));

	TEST(a1);
	TEST(a2);
	TEST(a3);
	TEST(a4);
	TEST(a5);

	return 0;
}
