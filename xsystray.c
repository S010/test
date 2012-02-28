#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <err.h>

#include <X11/Xlib.h>

extern char *optarg;

void usage(void);
int xsystray(int win_x, int win_y, int vertical_layout, int opposite_grow_dir,
    int icon_size);
Atom get_systray_selection_atom(Display *dpy);

int
main(int argc, char **argv)
{
	int vertical_layout = 0;
	int opposite_grow_dir = 0; /* in vertical layout: grow up;
				  in horizontal layout: grow left;
				 */
	int icon_size = 22;
	int x = 0, y = 0;
	int ch;

	while ((ch = getopt(argc, argv, "hvosp")) != -1) {
		switch (ch) {
		case 'v':
			++vertical_layout;
			break;
		case 'o':
			++opposite_grow_dir;
			break;
		case 's':
			icon_size = atoi(optarg);
			break;
		case 'p':
			if (sscanf(optarg, "%dx%d", &x, &y) == 0) {
				usage();
				return 1;
			}
			break;
		case 'h':
		default:
			usage();
			return 1;
			break;
		}
	}

	return xsystray(vertical_layout, opposite_grow_dir, icon_size);


}

void
usage(void)
{
	printf("usage: xsystray [-v] [-o] [-s <int>] [-p <int>x<int>]\n");
}

int
xsystray(int tray_win_x, int tray_win_y, int vertical_layout,
    int opposite_grow_dir, int icon_size)
{
	Display *dpy;
	Window tray_window, sel_window;
	GC gc;
	XEvent e;
	Atom selection_atom, opcode_atom;

	enum systray_opcodes {
		SYSTEM_TRAY_REQUEST_DOCK,
		SYSTEM_TRAY_BEGIN_MESSAGE,
		SYSTEM_TRAY_CANCEL_MESSAGE,
	};

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL)
		errx(1, "failed to open X display");

	selection_atom = get_selection_atom(dpy);
	tray_window = open_tray_window(dpy, tray_win_x, tray_win_y);
	if (acquire_systray_selection(Display *dpy, Atom selection_atom))
		errx("failed to acquire ownership of systray selection");

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

Atom
get_systray_selection_atom(Display *dpy)
{
	char buf[sizeof("_NET_SYSTEM_TRAY_Sxxx")];
	size_t bufsize = sizeof(buf);
	Atom atom;

	snprintf(buf, bufsize, "_NET_SYSTEM_TRAY_S%d", DefaultScreen(dpy));
	atom = XInternAtom(dpy, buf, False);

	return atom;
}

int
acquire_systray_selection(Display *dpy, Atom selection_atom)
{
	if (XGetSelectionOwner(dpy, selection_atom) != None)
		return -1;
	XSetSelectionOwner(dpy, selection_atom, 

}

Window
open_tray_window(Display *dpy, int x, int y)
{
	Window window;

	window = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), x, y, 1, 1, 0,
	    BlackPixel(dpy, DefaultScreen(dpy)),
	    WhitePixel(dpy, DefaultScreen(dpy)));
	XSelectInput(dpy, window, StructureNotifyMask | PropertyChangeMask);

	return window;
}
