all: gicon xsystray gui proc

gicon: gicon.c
	$(CC) -o $@ $< `pkg-config --cflags --libs gtk+-3.0`

xsystray: xsystray.c
	$(CC) -o $@ $< `pkg-config --cflags --libs x11`

gui: gui.cpp
	$(CXX) -std=c++11 -o $@ $< `pkg-config --cflags --libs gtk+-3.0`

proc: proc.cpp
	$(CXX) -std=c++11 -o $@ $<

clean:
	rm -f *.o gicon xsystray
