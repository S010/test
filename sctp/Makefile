CFLAGS += -O2 -Wall -Wextra -std=c11
LDFLAGS += -lsctp

.PHONY: all clean

all: client server

clean:
	rm -f *.o client server

client: client.o
	$(CC) $(LDFLAGS) -o $@ $<

server: server.o
	$(CC) $(LDFLAGS) -o $@ $<
