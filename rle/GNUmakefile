CC = clang
CFLAGS = -Wall -std=c99 -O2

all: bin/ brle

bin/:
	mkdir -p $@

brle: src/main.o src/brle.o
	$(CC) -o bin/$@ $^

clean:
	rm -rf bin/ src/*.o
