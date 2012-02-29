#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <err.h>

#include <X11/Xlib.h>

static void	 usage(void);
static int	 xsystray(int, int, int, int, int);
static Atom	 create_tray_atom(Display *);
static int	 acquire_systray_selection(Display *, Atom, Window);
static Window	 open_tray_window(Display *, int, int);
static Window	 open_sel_owner(Display *, Window);
static void	 notify_systray_appeared(Display *, Atom, Window);
static int	 same_screen(Display *, Window);
static void	 xembed_notify_embedded(Display *, Window, Window);
static void	 update_geometry(Display *, Window);
static void	 arrange_children(Display *, Window);
static void	 show_tray(Display *, Window);

int
main(int argc, char **argv)
{
	extern char	*optarg;
	int		 vertical_layout = 0;
	int		 opposite_grow_dir = 0;
	int		 icon_size = 22;
	int		 x = 0, y = 0;
	int		 ch;

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

	return xsystray(x, y, vertical_layout, opposite_grow_dir, icon_size);
}

static void
usage(void)
{
	printf("usage: xsystray [-v] [-o] [-s <int>] [-p <int>x<int>]\n");
}

static int
xsystray(int x, int y, int vertical_layout, int opposite_grow_dir,
    int icon_size)
{
	Display	*display;
	Atom	 tray_atom, opcode_atom;
	Window	 tray, sel_owner, icon;
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

	tray = open_tray_window(display, x, y);
	//XMapWindow(display, tray);
	sel_owner = open_sel_owner(display, tray);
	if (acquire_systray_selection(display, tray_atom, sel_owner))
		errx(1, "failed to acquire ownership of systray selection");
	notify_systray_appeared(display, tray_atom, sel_owner);

	for ( ;; ) {
		XNextEvent(display, &e);

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
			xembed_notify_embedded(display, sel_owner, icon);
			/* TODO
			 * resize to icon_size
			 * map if unmapped
			XMapWindow(display, icon);
			*/
			arrange_children(display, tray);
			update_geometry(display, tray);
			/* TODO
			 * if we have >0 icons, map the tray window
			 */
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

static int
acquire_systray_selection(Display *display, Atom tray_atom, Window owner)
{
	if (XGetSelectionOwner(display, tray_atom) != None)
		return -1;
	XSetSelectionOwner(display, tray_atom, owner, CurrentTime);
	return 0;
}

static Window
open_tray_window(Display *display, int x, int y)
{
	Window	 window;

	window = XCreateSimpleWindow(display, DefaultRootWindow(display),
	    x, y, 40, 40, 0,
	    BlackPixel(display, DefaultScreen(display)),
	    WhitePixel(display, DefaultScreen(display)));

	XSelectInput(display, window,
	    ButtonPressMask |
	    ButtonReleaseMask |
	    ExposureMask |
	    PropertyChangeMask |
	    SubstructureNotifyMask |
	    SubstructureRedirectMask);

	return window;
}

/* open the window that will own the systray selection */
static Window
open_sel_owner(Display *display, Window parent)
{
	Window	 window;

	window = XCreateSimpleWindow(display, parent, 0, 0, 1, 1, 0,
	    BlackPixel(display, DefaultScreen(display)),
	    WhitePixel(display, DefaultScreen(display)));

	XSelectInput(display, window, SubstructureNotifyMask);

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
xembed_notify_embedded(Display *display, Window sel_window, Window window)
{
	XEvent	 e;
	enum {
		XEMBED_EMBEDDED_NOTIFY,
	};

	memset(&e, 0, sizeof(e));

	e.xclient.type = ClientMessage;
	e.xclient.message_type = XInternAtom(display, "_XEMBED", False);
	e.xclient.display = display;
	e.xclient.window = window;
	e.xclient.format = 32;
	e.xclient.data.l[0] = CurrentTime;
	e.xclient.data.l[1] = XEMBED_EMBEDDED_NOTIFY;
	e.xclient.data.l[2] = 0; /* protocol version */
	e.xclient.data.l[3] = sel_window;

	XSendEvent(display, window, False, NoEventMask, &e);
}

static void
update_geometry(Display *display, Window tray)
{
}

static void
arrange_children(Display *display, Window tray)
{
}

static void
show_tray(Display *display, Window tray)
{
	XWindowAttributes	 attrs;

	XGetWindowAttributes(display, tray, &attrs);
	if (attrs.map_state & IsUnmapped)
		XMapWindow(display, tray);
}
