	.data
	.text
	.global __start
__start:
	li $a0, 88
	li $v0, 4001
	syscall
