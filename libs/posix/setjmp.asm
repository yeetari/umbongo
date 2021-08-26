; int setjmp(jmp_buf rdi)
global setjmp
setjmp:
    mov [rdi + 0], rbx
    mov [rdi + 8], r12
    mov [rdi + 16], r13
    mov [rdi + 24], r14
    mov [rdi + 32], r15
    mov [rdi + 40], rbp
    lea rdx, [rsp + 8]
    mov [rdi + 48], rdx
    mov rdx, [rsp]
    mov [rdi + 56], rdx
    xor eax, eax
    ret

; __attribute__((noreturn)) void longjmp(jmp_buf rdi, int rsi)
global longjmp
longjmp:
    mov eax, esi
    test eax, eax
    jnz .not_zero
    inc eax

.not_zero:
    mov rbx, [rdi + 0]
    mov r12, [rdi + 8]
    mov r13, [rdi + 16]
    mov r14, [rdi + 24]
    mov r15, [rdi + 32]
    mov rbp, [rdi + 40]
    mov rsp, [rdi + 48]
    jmp [rdi + 56]
