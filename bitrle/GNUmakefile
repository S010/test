CC = gcc
CFLAGS = -Wall -std=c99 -g
LDFLAGS = -g

all: bitrle

bitrle: bitrle.o
	$(CC) -o $@ $^

clean:
	rm -f *.o bitrle
