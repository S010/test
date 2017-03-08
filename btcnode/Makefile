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

all: btcnode

test: btcnode
	./btcnode
	hexdump -C btcnode.out

btcnode: main.o
	$(CC) $(LDFLAGS) -o $@ $<

clean:
	rm -f *.o btcnode
