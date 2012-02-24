#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <err.h>

#include <X11/Xlib.h>

int
main(int argc, char **argv)
{
	Display *dpy;
	Window window;
	GC gc;
	XEvent e;
	Atom atom;

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL)
		errx(1, "failed to open X display");

	atom = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
	window = XGetSelectionOwner(dpy, atom);
	if (window != None) {
		fprintf(stderr, "systray selection ownership already taken");
		exit(1);
	}

	window = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0,
	    BlackPixel(dpy, DefaultScreen(dpy)),
	    WhitePixel(dpy, DefaultScreen(dpy)));

	/* ... */

	XFlush(dpy);

	XCloseDisplay(dpy);

	return 0;
}
