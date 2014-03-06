CFLAGS=-g -O0 -Wall

.PHONY: run

all: run

test: test.o
	$(CC) -o test test.o

run: test
	env LD_DEBUG=LD_DEBUG ./test > test.ld.out 2>&1 || true

clean:
	rm -f *.o test *.core *.out
