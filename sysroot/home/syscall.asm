bits 64

SYS_EXIT  equ 9
SYS_WRITE equ 25

section .rodata
string: db "Hello, world", 0x0a
.len: equ $ - string

section .text
global _start
_start:
    mov rax, SYS_WRITE
    mov rdi, 1
    lea rsi, [rel string]
    mov rdx, string.len
    syscall
    mov rax, SYS_EXIT
    xor rdi, rdi
    syscall
