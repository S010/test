

void
line(unsigned char *buf, size_t bufsize, int x0, int y0, int x1, int y1)
{
	int	 dx;
	int	 dy;
	int	 sx;
	int	 sy;
	int	 error;
	int	 error2;

	dx = abs(x1 - x0);
	dy = abs(y1 - y0);
	error = dx - dy;

	if (x0 < x1)
		sx = 1;
	else
		sx = -1;

	if (y0 < y1)
		sy = 1;
	else
		sy = -1;

#define PLOT(x, y) \
	// plot a point

	for (;;) {
		PLOT(x0, y0);
		if (x0 == x1 && y0 == y1)
			break;
		error2 = error * 2;
		if (error2 > -dy) {
			error = error - dy;
			x0 = x0 + sx;
		}
		if (x0 == x1 && y0 == y1) {
			PLOT(x0, y0);
			break;
		}
		if (error2 < dx) {
			error = error + dx;
			y0 = y0 + sy;
		}
	}

#undef PLOT
}
