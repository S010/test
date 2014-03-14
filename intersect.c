#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define min(x, y)	((x) < (y)) ? (x) : (y)

struct point {
	int	 x;
	int	 y;
};

// Line segment
struct lineseg {
	struct point	 pt[2];
};

// Return true and put the coordinates of intersection point into P if
// line segments L1 and L2 intersect.
static bool
lineseg_intersect(struct lineseg *L1, struct lineseg *L2, struct point *P)
{
	int	 x0;
	int	 x0l;
	int	 y0;
	int	 y0l;
	int	 x1;
	int	 x1l;
	int	 y1;
	int	 y1l;

	x0 = min(L1->pt[0].x, L1->pt[1].x);
	x0l = abs(L1->pt[0].x - L1->pt[0].x);
	x1 = min(L2->pt[0].x, L2->pt[1].x);
	x1l = abs(L2->pt[0].x - L2->pt[1].x);

	if (x0 < x1 && x0+x0l < x1)
		return false;
	if (x0 > x1+x1l)
		return false;

	// TODO...

	return true;
}

int
main(int argc, char **argv)
{
	struct lineseg	 L1;
	struct lineseg	 L2;
	struct point	 P;
	bool		 result;

	if (argc < 2)
		return EXIT_SUCCESS;

	sscanf(argv[1],
	    " %d %d %d %d %d %d %d %d ",
	    &L1.pt[0].x,
	    &L1.pt[0].y,
	    &L1.pt[1].x,
	    &L1.pt[1].y,
	    &L2.pt[0].x,
	    &L2.pt[0].y,
	    &L2.pt[1].x,
	    &L2.pt[1].y);
	result = lineseg_intersect(&L1, &L2, &P);

	if (result)
		puts("lines may be intersecting");
	else
		puts("lines do not intersect");

	return EXIT_SUCCESS;
}
