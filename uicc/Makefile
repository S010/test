CFLAGS = -Wall -O2 -Wextra -std=c99 -D_DEFAULT_SOURCE -D_BSD_SOURCE

.PHONY: all clean

all: uicc

clean:
	rm -f *.o uicc

uicc: uicc.o
	$(CC) -o uicc uicc.o

