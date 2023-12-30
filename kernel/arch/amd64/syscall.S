.file "syscall.S"
.text

.extern syscall_handler
.globl syscall_entry
.type syscall_entry, "function"
syscall_entry:
    // Store user stack pointer and load kernel stack pointer.
    mov %rsp, %gs:16
    mov %gs:8, %rsp
    pushq %gs:16

    // Push all GP registers.
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rdi
    push %rsi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    // Pass a pointer to the syscall frame, as well as the current thread.
    mov %rsp, %rdi
    mov %gs:24, %rsi
    call syscall_handler

    // Pop back GP registers.
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rsi
    pop %rdi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    pop %rsp
    sysretq
