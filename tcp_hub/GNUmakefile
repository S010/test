MISC_DIR=$(HOME)/Source/misc

CC=clang
CFLAGS+=-Wall
CFLAGS+=-std=c99
CFLAGS+=-I$(MISC_DIR)/clib/include
LDFLAGS+=-L$(MISC_DIR)/clib/lib -lclib

all: tcp_hub echo_server

tcp_hub: tcp_hub.o
	$(CC) $< $(LDFLAGS) -o $@ 

echo_server: echo_server.o
	$(CC) $< $(LDFLAGS) -o $@ 

clean:
	rm -f tcp_hub echo_server *.o
