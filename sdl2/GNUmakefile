CFLAGS = $(shell sdl2-config --cflags)
CFLAGS += -I/usr/local/include
CFLAGS += -Wall
CFLAGS += -std=c99
CFLAGS += -O2
CFLAGS += -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk/System/Library/Frameworks/OpenGL.framework/Headers
LDFLAGS = $(shell sdl2-config --libs) -framework OpenGL

all: init hello cursor inertia freefall surface rotozoom filter line mandelbrot mandelbrot2 glline glcolors

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

filter: filter.o
	${CC} ${LDFLAGS} -lm -o filter filter.o

line: line.o
	${CC} ${LDFLAGS} -lm -o line line.o

mandelbrot: mandelbrot.o
	${CC} ${LDFLAGS} -lm -o mandelbrot mandelbrot.o

mandelbrot2: mandelbrot2.o
	${CC} ${LDFLAGS} -lm -o mandelbrot2 mandelbrot2.o

glline: glline.o
	${CC} ${LDFLAGS} -lm -o glline glline.o

glcolors: glcolors.o
	${CC} ${LDFLAGS} -lm -o glcolors glcolors.o

clean:
	rm -f *.o init hello cursor inertia freefall surface rotozoom filter line mandelbrot mandelbrot2 glline glcolors
