PROGNAME=crash

CFLAGS+=-O0 -D$(PROGNAME)
LDFLAGS+=-lpthread

all: $(PROGNAME)

crash: crash.o
	$(CC) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(PROGNAME) *.o
