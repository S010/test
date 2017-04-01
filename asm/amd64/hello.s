bits 64

message db 'Hello, world!',10
msglen equ $-message

exit equ 60
write equ 1

global _start
_start:
pop rsi
and rsp,~0xf
push rax
push rsp
call main
hlt

global main
main:
mov rax,write
mov rdi,1
mov rsi,message
mov rdx,msglen
syscall

mov rax,exit
mov rdi,0
syscall

