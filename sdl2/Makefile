CFLAGS != sdl2-config --cflags
CFLAGS += -Wall
CFLAGS += -std=c99
LDFLAGS != sdl2-config --libs

all: init hello cursor

init: init.o
	${CC} ${LDFLAGS} -o init init.o

hello: hello.o
	${CC} ${LDFLAGS} -o hello hello.o

cursor: cursor.o
	${CC} ${LDFLAGS} -o cursor cursor.o

clean:
	rm -f *.o init hello cursor
