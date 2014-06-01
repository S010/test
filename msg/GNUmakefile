CC=gcc
CFLAGS=-g -Wall -O2 -std=c99

all: master slave

master: msg.o master.o
	$(CC) -o $@ $^

slave: msg.o slave.o
	$(CC) -o $@ $^

clean:
	rm -f *.o master slave
