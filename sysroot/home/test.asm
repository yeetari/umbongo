bits 64

section .rodata
string:
    db "Hello, world", 0

section .text
extern puts
global main
main:
    mov rdi, string
    call puts
    xor eax, eax
    ret
