CFLAGS = -Wall -O2 -Wextra -std=c99

.PHONY: all clean

all: uicc

clean:
	rm -f *.o uicc

uicc: uicc.o
	$(CC) -o uicc uicc.o

