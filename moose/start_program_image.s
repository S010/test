	.file	"start.s"
	.text

	.global end

	.align  2
ivt: /* Image Vector Table */
	.word	0x402000d1	/* header */
	.word   start		/* entry */
	.word   0		/* reserved */
	.word   0		/* abs addr of DCD */
	.word   boot_data	/* abs addr of boot_data */
	.word   ivt		/* self */
	.word   0		/* abs addr of CSF */
	.word   0		/* reserved */

	.align  2
boot_data:
	.word	ivt		/* abs addr of program image */
	.word	end - ivt	/* program image size */
	.word	0		/* plugin? */

	.align	2
	.global	start
start:
	ldr	sp, =stack_ptr
	bl	main
	b	.
stack_ptr = end + 0x1000	/* 4KiB stack */
