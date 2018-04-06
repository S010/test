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

all: bin/ bin/btcd

bin/:
	mkdir -p bin

bin/btcd: src/main.o src/protocol.o src/peer.o src/util.o
	$(CC) $(LDFLAGS) -o $@ $^

test: bin/btcd
	./bin/btcd 2>&1 | tee out.txt

clean:
	rm -rf src/*.o bin out.txt

analyze: src/main.c src/protocol.c src/peer.c src/util.c
	clang-check -analyze $^
