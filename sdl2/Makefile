CFLAGS != sdl2-config --cflags
CFLAGS += -Wall
CFLAGS += -std=c99
LDFLAGS != sdl2-config --libs

all: init hello cursor inertia freefall

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

clean:
	rm -f *.o init hello cursor inertia freefall
