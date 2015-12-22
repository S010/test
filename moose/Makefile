AS = arm as
CC = arm gcc
LD = arm ld
OBJCOPY = arm objcopy
CFLAGS = -Wall -Werror -std=c99
CFLAGS += -I.
CFLAGS += -mcpu=cortex-a9

.PHONY: help clean led_program_image led_flat

help:
	@echo make led_program_image -- build a program image of led
	@echo make led_flat -- build a flat binary of led 

clean:
	rm -f *.o *.bin *.elf

led_program_image: OBJECTS = start_program_image.o end.o led.o
led_program_image: start_program_image.o
led_program_image: end.o
led_program_image: led.o
led_program_image: led

led_flat: OBJECTS = start_flat.o end.o led.o
led_flat: start_flat.o
led_flat: end.o
led_flat: led.o
led_flat: led

led:
	$(CC) -nostartfiles -nodefaultlibs -nostdlib -s -T led.lds -o led.elf $(OBJECTS)
	$(OBJCOPY) -O binary led.elf led.bin
