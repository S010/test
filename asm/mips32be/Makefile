.PHONY: all clean

PROGRAMS = exit1 exit2 hello

all: $(PROGRAMS)

exit1.o: exit1.s
	mips as -o $@ $^

exit1: exit1.o
	mips ld -o $@ $^

exit2: exit2.s
	mips gcc -nostdlib -o $@ $^

hello: hello.s
	mips gcc --disable-shared -nostdlib -o $@ $^

clean:
	rm -f $(PROGRAMS) *.o
