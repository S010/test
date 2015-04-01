CFLAGS += -Wall -Wpedantic -Wno-switch -std=c99 -D_XOPEN_SOURCE=600
LDFLAGS += -lcurses


.PHONY = all clean

all: bin/ bin/tchess

clean:
	rm -rf bin src/*.o

bin/tchess: src/main.o
	$(CC) $(LDFLAGS) -o $@ $<

bin/:
	mkdir -p $@

