	.file	"start.s"
	.text

	.global end

	.align	2
	.global	start
start:
	ldr	sp, =stack_ptr
	bl	main
	b	.
stack_ptr = end + 0x100	/* 256 byte stack */
