ENTRY(start)
SECTIONS {
	.text 0x900000 : {
            start_program_image.o(.text)
            led.o(.text)
            end.o(.text)
        }
}
