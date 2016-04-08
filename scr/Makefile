CFLAGS += $(shell pkg-config --cflags libusb-1.0)
CFLAGS += -Wall -Werror -Wextra -std=c99
LDFLAGS += $(shell pkg-config --libs libusb-1.0)

all: scr

scr: scr.o
	$(CC) $(LDFLAGS) -o $@ $<
