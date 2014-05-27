CC = clang
CFLAGS = -Wall -std=c99

all: zip

zip: zip.o
	$(CC) -o $@ $^

clean:
	rm -f *.o zip
