CFLAGS += \
	  -std=c11 \
	  -Wall \
	  -Wextra \
	  -Werror \
	  -pedantic \
	  -O3

LDFLAGS += \
	   -lcrypto

.PHONY: all clean

all: btcnode

btcnode: main.o
	$(CC) $(LDFLAGS) -o $@ $<

clean:
	rm -f *.o btcnode
