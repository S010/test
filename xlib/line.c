#include <X11/Xlib.h>
#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#ifndef PI
#define PI (3.14159265358979323846)
#endif

const int	 windowHeight = 500;
const int	 windowWidth = 500;

typedef struct {
	int	 x;
	int	 y;
} Vertex;

typedef struct {
	unsigned	 n; /* number of vertices */
	Vertex	 v[];
} Figure;

Display		*dpy;
Window		 w;
GC		 gc;
Vertex		 origin;

Figure *
AllocFigure(unsigned n)
{
	Figure *f = malloc(sizeof(Figure) + n * sizeof(Vertex));
	if (f == NULL)
		err(1, "calloc failed");
	f->n = n;
	return f;
}

Figure *
CopyFigure(Figure *dst, Figure *src)
{
	memcpy(dst, src, sizeof(*src) + src->n * sizeof(Vertex));
}

Figure *
DupFigure(Figure *f)
{
	Figure	*g = AllocFigure(f->n);
	CopyFigure(g, f);
	return g;
}


void
DrawFigure(Figure *f)
{
	unsigned	 j;
	unsigned	 k;
	for (unsigned i = 0; i <= f->n; i++) {
		/* some magic due to need to connect last and first vertices */
		j = i % f->n;
		k = (i + 1) % f->n;
		XDrawLine(dpy, w, gc,
			origin.x + f->v[j].x, origin.y - f->v[j].y,
			origin.x + f->v[k].x, origin.y - f->v[k].y);
	}
}

void
RotateFigure(Figure *f, double angle_delta)
{
	Vertex	*v;
	double	 x;
	double	 y;
	double	 d;
	double	 angle;

	for (v = f->v; v != f->v + f->n; v++) {
		x = v->x;
		y = v->y;
		d = sqrt(x*x + y*y);
		if (d == 0.0)
			continue;
		angle = acos(x / d) + angle_delta;
		v->x = cos(angle) * d;
		v->y = sin(angle) * d;
		printf("(%f, %f) -> (%d, %d) (d=%f)\n", x, y, v->x, v->y, d);
	}
}

void
Draw(void)
{
	const int	 side = 80;
	Figure		*square;
	Figure		*square_t;

	square = AllocFigure(4);
	square->v[0] = (Vertex){ 0, 0 };
	square->v[1] = (Vertex){ 0, side };
	square->v[2] = (Vertex){ side, side };
	square->v[3] = (Vertex){ side, 0 };
	DrawFigure(square);

	square_t = DupFigure(square);
	RotateFigure(square_t, PI / 6.0);
	DrawFigure(square_t);

	CopyFigure(square_t, square);
	RotateFigure(square_t, PI / 5.0);
	DrawFigure(square_t);

	CopyFigure(square_t, square);
	RotateFigure(square_t, PI / 4.0);
	DrawFigure(square_t);
}

int
main(int argc, char **argv)
{
	unsigned long	 blackColor;
	unsigned long	 whiteColor;
	XEvent		 e;
	bool		 exposed = false;

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL)
		errx(1, "XOpenDisplay failed");

	blackColor = BlackPixel(dpy, DefaultScreen(dpy));
	whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

	w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, windowWidth, windowHeight, 0, blackColor, blackColor);
	XSelectInput(dpy, w, StructureNotifyMask|ExposureMask);
	XMapWindow(dpy, w);

	Atom WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, w, &WM_DELETE_WINDOW, 1);

	gc = XCreateGC(dpy, w, 0, NULL);

	XSetForeground(dpy, gc, whiteColor);

	origin = (Vertex){ windowWidth / 2, windowHeight / 2 };

	XFlush(dpy);

	for (bool quit = false; !quit;) {
		XNextEvent(dpy, &e);
		switch (e.type) {
		case ConfigureNotify:
			origin = (Vertex){ e.xconfigure.width / 2, e.xconfigure.height / 2 };
			XClearWindow(dpy, w);
			if (!exposed)
				break;
			/* passthrough */
		case Expose:
			exposed = true;
			Draw();
			XFlush(dpy);
			break;
		case ClientMessage:
			/* we can only get WM_DELETE_WINDOW */
			quit = true;
			break;
		}
	}

	XFreeGC(dpy, gc);
	XCloseDisplay(dpy);

	return 0;
}
