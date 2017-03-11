CFLAGS += \
	  -std=c11 \
	  -Wall \
	  -Wextra \
	  -Werror \
	  -pedantic \
	  -g \
	  -O3

LDFLAGS += \
	   -lcrypto

.PHONY: all test clean

all: bin/ bin/btcnode

bin/:
	mkdir -p bin

bin/btcnode: src/main.o src/protocol.o
	$(CC) $(LDFLAGS) -o $@ $^

test: bin/btcnode
	./bin/btcnode
	hexdump -C btcnode.out

clean:
	rm -rf src/*.o bin btcnode.out
