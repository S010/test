CFLAGS += \
	  -std=c11 \
	  -Wall \
	  -Wextra \
	  -Werror \
	  -pedantic \
	  -g \
	  -O3 \
	  -D_DEFAULT_SOURCE \
	  -D_BSD_SOURCE \
	  -D_POSIX_C_SOURCE=201112L

LDFLAGS += \
	   -lcrypto

.PHONY: all test clean

all: bin/ bin/btcnode

bin/:
	mkdir -p bin

bin/btcnode: src/main.o src/protocol.o src/peer.o src/util.o
	$(CC) $(LDFLAGS) -o $@ $^

test: bin/btcnode
	./bin/btcnode

clean:
	rm -rf src/*.o bin btcnode.out
