CC = gcc
CFLAGS = -Wall -std=c99 -O2

.PHONY: clean

all: coredumper

coredumper: main.o
	$(CC) -o $@ $<

clean:
	rm -f *.o coredumper
