	.equ	exit, 4001
	.equ	write, 4004

	.globl __start
__start:
	li	$v0, write
	li	$a0, 1
	la	$a1, msg
	li	$a2, 14
	syscall

	li	$v0, exit
	move	$a0, $v0
	syscall


	.data
msg:	.asciz	"Hello, world!\n"
