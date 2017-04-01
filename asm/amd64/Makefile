.PHONY: all clean

all: hello

hello: hello.o
	$(CC) -O3 -nostdlib -static -o $@ $^
	strip $@

hello.o: hello.s
	yasm -f elf64 -m amd64 $^

clean:
	rm hello hello.o
