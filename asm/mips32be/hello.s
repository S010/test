	.equ	exit, 4001
	.equ	write, 4004

	.globl __start
__start:
	.ent __start
	.frame	$sp, 32, $ra
	.set noreorder
	.cpload		$t9
	.set reorder
	subu	$sp, $sp, 32
	sw	$ra, 28($sp)
	.cprestore	24

	la	$a1, msg
	li	$v0, write
	li	$a0, 1
	li	$a2, 14
	syscall

	li	$v0, exit
	move	$a0, $v0
	syscall

	.end __start


	.data
msg:	.asciz	"Hello, world!\n"
