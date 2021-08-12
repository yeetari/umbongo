bits 64

section .rodata
string: db "Hello, world", 0

section .text
extern puts
global main
main:
    lea rdi, [rel string]
    call puts
    xor eax, eax
    ret
