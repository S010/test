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

    dpy = XOpenDisplay(NULL);
    if (dpy == NULL)
        errx(1, "failed to open X display");

    window = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 400, 300,
      0, BlackPixel(dpy, DefaultScreen(dpy)), WhitePixel(dpy, DefaultScreen(dpy)));
    XSelectInput(dpy, window, StructureNotifyMask);
    XMapWindow(dpy, window);

    gc = XCreateGC(dpy, window, 0, NULL);

    XSetForeground(dpy, gc, BlackPixel(dpy, DefaultScreen(dpy)));
    
    for ( ;; ) {
        XNextEvent(dpy, &e);
        if (e.type == MapNotify)
            break;
    }

    XDrawLine(dpy, window, gc, 10, 60, 180, 20);

    XFlush(dpy);

    sleep(3);

    XCloseDisplay(dpy);

    return 0;
}
