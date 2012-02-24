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
	Atom sel_atom, opcode_atom;

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL)
		errx(1, "failed to open X display");

	sel_atom = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);

	if (XGetSelectionOwner(dpy, sel_atom) != None) {
		fprintf(stderr, "systray selection ownership already taken\n");
		exit(1);
	}
	window = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0,
	    BlackPixel(dpy, DefaultScreen(dpy)),
	    WhitePixel(dpy, DefaultScreen(dpy)));
	XSelectInput(dpy, window, StructureNotifyMask | PropertyChangeMask);
	XSetSelectionOwner(dpy, sel_atom, window, CurrentTime);

	opcode_atom = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
	for ( ;; ) {
		XNextEvent(dpy, &e);

		if (e.type == ClientMessage
		    && e.xclient.message_type == opcode_atom
		    && e.xclient.format == 32) {
			printf("received systray opcode\n");
		}
	}

	XCloseDisplay(dpy);

	return 0;
}
