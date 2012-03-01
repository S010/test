#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <err.h>

#include <X11/Xlib.h>

static void	 usage(void);
static int	 xsystray(int);
static Atom	 create_tray_atom(Display *);
static void	 acquire_systray_selection(Display *, Atom, Window);
static Window	 open_tray_window(Display *, int);
static void	 notify_systray_appeared(Display *, Atom, Window);
static int	 same_screen(Display *, Window);
static void	 xembed_notify_embedded(Display *, Window, Window);
static void	 update_geometry(Display *, Window, int);
static void	 arrange_children(Display *, Window, int);
static int	 get_n_children(Display *, Window);

int
main(int argc, char **argv)
{
	extern char	*optarg;
	int		 icon_size = 24;
	int		 ch;
	int		 rc;

	while ((ch = getopt(argc, argv, "hvosp")) != -1) {
		switch (ch) {
		case 's':
			icon_size = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			return 1;
			break;
		}
	}

	rc = xsystray(icon_size);

	return rc;
}

static void
usage(void)
{
	printf("usage: xsystray [-s <int>]\n");
	printf("  -s -- set the size of the icons (24 by default)\n");
}

static int
xsystray(int icon_size)
{
	Display	*display;
	Atom	 tray_atom, opcode_atom;
	Window	 tray, icon;
	XEvent	 e;
	long	 opcode;
	enum opcodes {
		SYSTEM_TRAY_REQUEST_DOCK,
		SYSTEM_TRAY_BEGIN_MESSAGE,
		SYSTEM_TRAY_CANCEL_MESSAGE,
	};

	display = XOpenDisplay(NULL);
	if (display == NULL)
		errx(1, "failed to open X display");

	tray_atom = create_tray_atom(display);
	opcode_atom = XInternAtom(display, "_NET_SYSTEM_TRAY_OPCODE", False);

	tray = open_tray_window(display, icon_size);
	acquire_systray_selection(display, tray_atom, tray);
	notify_systray_appeared(display, tray_atom, tray);

	for ( ;; ) {
		XNextEvent(display, &e);

		printf("XEvent, type=%d\n", (int) e.type);

		if (e.type == DestroyNotify && e.xdestroywindow.window != tray) {
			arrange_children(display, tray, icon_size);
			update_geometry(display, tray, icon_size);
		}

		if (!(e.type == ClientMessage
		    && e.xclient.message_type == opcode_atom
		    && e.xclient.format == 32))
			continue;

		opcode = e.xclient.data.l[1];

		if (opcode == SYSTEM_TRAY_REQUEST_DOCK) {
			printf("system_tray_request_dock\n");
			icon = (Window) e.xclient.data.l[2];
			if (!same_screen(display, icon))
				continue;
			XReparentWindow(display, icon, tray, 0, 0);
			xembed_notify_embedded(display, tray, icon);
			XResizeWindow(display, icon, icon_size, icon_size);
			XMapWindow(display, icon);
			arrange_children(display, tray, icon_size);
			update_geometry(display, tray, icon_size);
		} else if (opcode == SYSTEM_TRAY_BEGIN_MESSAGE) {
			printf("system_tray_begin_message\n");
		} else if (opcode == SYSTEM_TRAY_CANCEL_MESSAGE) {
			printf("system_tray_cancel_message\n");
		}
	}

	XCloseDisplay(display);

	return 0;
}

static Atom
create_tray_atom(Display *display)
{
	char	 buf[sizeof("_NET_SYSTEM_TRAY_Sxxx")];
	size_t	 bufsize = sizeof(buf);
	Atom	 atom;

	snprintf(buf, bufsize, "_NET_SYSTEM_TRAY_S%d", DefaultScreen(display));
	atom = XInternAtom(display, buf, False);

	return atom;
}

static void
acquire_systray_selection(Display *display, Atom tray_atom, Window owner)
{
	if (XGetSelectionOwner(display, tray_atom) != None)
		errx(1, "failed to acquire systray selection ownership");
	XSetSelectionOwner(display, tray_atom, owner, CurrentTime);
}

static Window
open_tray_window(Display *display, int icon_size)
{
	Window	 window;

	window = XCreateSimpleWindow(display, DefaultRootWindow(display),
	    0, 0, 1, icon_size, 0,
	    WhitePixel(display, DefaultScreen(display)),
	    BlackPixel(display, DefaultScreen(display)));

	XSelectInput(display, window,
	    ButtonPressMask |
	    ButtonReleaseMask |
	    ExposureMask |
	    PropertyChangeMask |
	    SubstructureNotifyMask |
	    SubstructureRedirectMask);

	XMapWindow(display, window);

	return window;
}

static void
notify_systray_appeared(Display *display, Atom tray_atom, Window sel_window)
{
	XEvent	 e;

	memset(&e, 0, sizeof(e));

	e.xclient.type = ClientMessage;
	e.xclient.message_type = XInternAtom(display, "MANAGER", False);
	e.xclient.window = DefaultRootWindow(display);
	e.xclient.format = 32;
	e.xclient.data.l[0] = CurrentTime;
	e.xclient.data.l[1] = tray_atom;
	e.xclient.data.l[2] = sel_window;

	XSendEvent(display, DefaultRootWindow(display), False, StructureNotifyMask, &e);
}

static int
same_screen(Display *display, Window icon)
{
	XWindowAttributes	 attrs;

	if (!XGetWindowAttributes(display, icon, &attrs)) {
		warnx("failed to get icon window attributes");
		return 0;
	}

	if (DefaultScreen(display) == XScreenNumberOfScreen(attrs.screen))
		return 1;
	else
		return 0;
}

static void
xembed_notify_embedded(Display *display, Window embedder, Window embedded)
{
	XEvent	 e;
	enum {
		XEMBED_EMBEDDED_NOTIFY,
	};

	memset(&e, 0, sizeof(e));

	e.xclient.type = ClientMessage;
	e.xclient.message_type = XInternAtom(display, "_XEMBED", False);
	e.xclient.display = display;
	e.xclient.window = embedded;
	e.xclient.format = 32;
	e.xclient.data.l[0] = CurrentTime;
	e.xclient.data.l[1] = XEMBED_EMBEDDED_NOTIFY;
	e.xclient.data.l[2] = 0; /* protocol version */
	e.xclient.data.l[3] = embedder;

	XSendEvent(display, embedded, False, NoEventMask, &e);
}

static void
update_geometry(Display *display, Window tray, int icon_size)
{
	int	 width;

	width = icon_size * get_n_children(display, tray);
	if (width < 1)
		width = 1;
	XResizeWindow(display, tray, width, icon_size);

	/*
	 * TODO
	 * place the window in lower-right corner of the screen
	 */
}

static void
arrange_children(Display *display, Window tray, int icon_size)
{
	Window	 root, parent, *icon, *children;
	int	 x, n_children;

	XQueryTree(display, tray, &root, &parent, &children, &n_children);

	for (icon = children, x = 0; n_children-- > 0; ++icon, x += icon_size)
		XMoveWindow(display, *icon, x, 0);
	XFree(children);
}

static int
get_n_children(Display *display, Window window)
{
	Window	 root, parent, *children;
	int	 n;

	XQueryTree(display, window, &root, &parent, &children, &n);

	if (children != NULL)
		XFree(children);

	return n;
}

