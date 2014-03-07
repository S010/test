APOLLO=/home/slava/src/apollo_trunk
CC=$(APOLLO)/output/host/opt/ext-toolchain/bin/arm-equinox-linux-gnueabi-gcc

PROGNAME=crash

CFLAGS+=-O0
CFLAGS+=-DPROGNAME=$(PROGNAME)
#CFLAGS+=-g
LDFLAGS+=-lpthread

all: $(PROGNAME)

crash: crash.o
	$(CC) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(PROGNAME) *.o
