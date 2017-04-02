	.data
	.text
	.global __start
__start:
	.set noreorder
	.cpload		$t9
	.set reorder
	li $a0, 99
	li $v0, 4001
	syscall
