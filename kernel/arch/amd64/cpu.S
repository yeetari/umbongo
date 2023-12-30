.file "cpu.S"
.text

.globl load_gdt
.type load_gdt, "function"
load_gdt:
    // Load GDTR.
    lgdt (%rdi)

    // iret to switch code selector.
    mov %rsp, %rax
    push $0x10 // ss (kernel data)
    push %rax  // rsp
    pushfq     // rflags
    push $0x08 // cs (kernel code)
    push $1f   // rip
    iretq

1:
    // Zero data selectors (excluding gs).
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs

    // Load task register.
    mov $0x28, %ax
    ltr %ax
    ret

.globl switch_now
.type switch_now, "function"
switch_now:
    pushq 168(%rdi) // ss
    pushq 160(%rdi) // rsp
    pushq 152(%rdi) // rflags
    pushq 144(%rdi) // cs
    pushq 136(%rdi) // rip
    mov 0(%rdi), %r15
    mov 8(%rdi), %r14
    mov 16(%rdi), %r13
    mov 24(%rdi), %r12
    mov 32(%rdi), %r11
    mov 40(%rdi), %r10
    mov 48(%rdi), %r9
    mov 56(%rdi), %r8
    mov 64(%rdi), %rbp
    mov 72(%rdi), %rsi
    mov 88(%rdi), %rdx
    mov 96(%rdi), %rcx
    mov 104(%rdi), %rbx
    mov 112(%rdi), %rax
    mov 80(%rdi), %rdi
    iretq
