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

	enum systray_opcodes {
		SYSTRAY_OPCODE_DOCK,
		SYSTRAY_OPCODE_BEGIN_MSG,
		SYSTRAY_OPCODE_CANCEL_MSG,
	};

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

	/* Notify existing systray icons that a WILD TRAY HAS APPEARED! */
	memset(&e, 0, sizeof(e));
	e.xclient.type = ClientMessage;
	e.xclient.message_type = XInternAtom(dpy, "MANAGER", False);
	e.xclient.display = dpy;
	e.xclient.window = DefaultRootWindow(dpy);
	e.xclient.format = 32;
	e.xclient.data.l[0] = CurrentTime;
	e.xclient.data.l[1] = sel_atom;
	e.xclient.data.l[2] = window;
	XSendEvent(dpy, DefaultRootWindow(dpy), False, StructureNotifyMask, &e);

	opcode_atom = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
	for ( ;; ) {
		XNextEvent(dpy, &e);

		if (e.type == ClientMessage
		    && e.xclient.message_type == opcode_atom
		    && e.xclient.format == 32) {
			switch (e.xclient.data.l[1]) {
			case SYSTRAY_OPCODE_DOCK:
				printf("received opcode dock, wid=%ld\n",
				    e.xclient.data.l[2]);
				break;
			case SYSTRAY_OPCODE_BEGIN_MSG:
				printf("received opcode begin msg\n");
				break;
			case SYSTRAY_OPCODE_CANCEL_MSG:
				printf("received opcode cancel msg\n");
				break;
			}
		}
	}

	XCloseDisplay(dpy);

	return 0;
}
