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

    Window w1, w2;
    int root_x, root_y;
    int x, y;
    unsigned int mask;
    Bool retval;


    dpy = XOpenDisplay(NULL);
    if (dpy == NULL)
        errx(1, "failed to open X display");

    retval = XQueryPointer(dpy, DefaultRootWindow(dpy), &w1, &w2, &root_x,
      &root_y, &x, &y, &mask);

    printf("same_screen=%d\nx=%d\ny=%d\n", retval, x, y);

    XCloseDisplay(dpy);

    return 0;
}
