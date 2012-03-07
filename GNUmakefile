all: gicon xsystray

gicon: gicon.c
	$(CC) -o $@ $< `pkg-config --cflags --libs gtk+-3.0`

xsystray: xsystray.c
	$(CC) -o $@ $< `pkg-config --cflags --libs x11`
