AS = arm as
CC = arm gcc
LD = arm ld
OBJCOPY = arm objcopy
CFLAGS = -Wall -Werror -Wextra -std=c99
CFLAGS += -Iinclude
CFLAGS += -Isrc/socs/imx6
CFLAGS += -mcpu=cortex-a9
CFLAGS += -O0
CFLAGS += -DBOARD_WANDBOARD -DSOC_IMX6

.PHONY: help clean led

help:
	@echo make led -- build led: lights up some LEDs

clean:
	rm -f *.bin *.elf src/boards/wandboard/*.o

led: led.bin

led.bin: led.elf
	$(OBJCOPY) -O binary led.elf led.bin

led.elf: src/boards/wandboard/start.o src/boards/wandboard/end.o src/boards/wandboard/wandboard.o src/boards/wandboard/led.o
	$(CC) -nostartfiles -nodefaultlibs -nostdlib -s -T src/boards/wandboard/link_script -o led.elf $^
