PROGNAME=crash

CFLAGS+=-O0
CFLAGS+=-DPROGNAME=$(PROGNAME)
CFLAGS+=-g
LDFLAGS+=-lpthread

all: $(PROGNAME)

crash: crash.o
	$(CC) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(PROGNAME) *.o
