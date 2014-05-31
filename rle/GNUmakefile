CC = gcc
CFLAGS = -Wall -std=c99 -O2

all: brle

brle: main.o brle.o
	$(CC) -o $@ $^

clean:
	rm -f *.o brle
