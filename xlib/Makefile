CFLAGS = -O2 -std=c99
LDFLAGS = -lX11 -lm

.PHONY: all clean

all: line

clean:
	rm -f line *.o

line: line.o
	$(CC) -o $@ $(LDFLAGS) $<
