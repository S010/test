MISC_DIR=/home/slava/src/misc

CC=gcc
CFLAGS+=-Wall
CFLAGS+=-std=c99
CFLAGS+=-I$(MISC_DIR)/clib/include
LDFLAGS+=-L$(MISC_DIR)/clib/lib -lclib

all: tcp_hub

tcp_hub: tcp_hub.o
	$(CC) $< $(LDFLAGS) -o $@ 

clean:
	rm -f tcp_hub *.o
