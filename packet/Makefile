CFLAGS = -std=c99 -D_BSD_SOURCE -D_DEFAULT_SOURCE

.PHONY: all clean

all: packet

clean:
	rm -f *.o packet

packet: main.o
	$(CC) -o packet main.c


