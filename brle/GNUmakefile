CC = gcc
CFLAGS = -Wall -std=c99 -O2
LDFLAGS = -g

all: bitrle

bitrle: bitrle.o
	$(CC) -o $@ $^

clean:
	rm -f *.o bitrle
