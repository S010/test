CFLAGS += -std=c11 -Wall -Wextra -Werror -pedantic -D_DEFAULT_SOURCE
CFLAGS += -O3 -g3

PROG = ubitcoind
SRC = $(shell ls *.c)
OBJ = $(SRC:%.c=%.o)

.PHONY: all distclean clean

all: .deps $(PROG)

clean:
	rm -f *.o $(PROG)

distclean: clean
	rm -f .deps

$(PROG): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

.deps: $(SRC)
	$(CC) -MM -o $@ $(SRC)

include .deps

