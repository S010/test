CFLAGS = $(shell sdl2-config --cflags)
CFLAGS += -Wall
CFLAGS += -std=c99
CFLAGS += -O2

# Mac OS X
#CFLAGS += -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk/System/Library/Frameworks/OpenGL.framework/Headers
#LDFLAGS = $(shell sdl2-config --libs) -framework OpenGL

# OpenBSD
CFLAGS += -I/usr/X11R6/include/GL
CFLAGS += $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs)
LDFLAGS += -L/usr/X11R6/lib -lGL

all: init hello cursor inertia freefall surface rotozoom line mandelbrot glcolors

init: init.o
	${CC} ${LDFLAGS} -o init init.o

hello: hello.o
	${CC} ${LDFLAGS} -o hello hello.o

cursor: cursor.o
	${CC} ${LDFLAGS} -o cursor cursor.o

inertia: inertia.o
	${CC} ${LDFLAGS} -o inertia inertia.o

freefall: freefall.o
	${CC} ${LDFLAGS} -o freefall freefall.o

surface: surface.o
	${CC} ${LDFLAGS} -o surface surface.o

rotozoom: rotozoom.o
	${CC} ${LDFLAGS} -lm -o rotozoom rotozoom.o

line: line.o
	${CC} ${LDFLAGS} -lm -o line line.o

mandelbrot: mandelbrot.o
	${CC} ${LDFLAGS} -lm -o mandelbrot mandelbrot.o

glcolors: glcolors.o
	${CC} ${LDFLAGS} -lm -o glcolors glcolors.o

clean:
	rm -f *.o init hello cursor inertia freefall surface rotozoom line mandelbrot glcolors
