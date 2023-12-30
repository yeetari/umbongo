.file "interrupt.S"
.text

.extern interrupt_handler
interrupt_entry:
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

    // Pass pointer to the register file on stack to allow modification in the handler.
    mov %rsp, %rdi
    call interrupt_handler

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

    // Cleanup error code and interrupt number.
    add $16, %rsp
    iretq

.altmacro
.macro INT num:req, err=0
    .globl int\num
    int\num:
    .ifeq \err
        // Push dummy error code.
        push $0
    .endif
        // Force generation of dword push.
        .byte 0x68
        .long \num
        jmp interrupt_entry
    .if \err
        // Generate nops for alignment.
        .nops 2
    .endif
.endm

.section ".interrupt_stubs", "ax"

// Exceptions (AMD64 System Programming Manual revision 3.41 section 8.2)
INT 0
INT 1
INT 2
INT 3
INT 4
INT 5
INT 6
INT 7
INT 8, 1
INT 9
INT 10, 1
INT 11, 1
INT 12, 1
INT 13, 1
INT 14, 1
INT 15
INT 16
INT 17, 1
INT 18
INT 19
INT 20
INT 21, 1
INT 22
INT 23
INT 24
INT 25
INT 26
INT 27
INT 28
INT 29
INT 30, 1
INT 31

// IRQs.
.set irq, 32
.rept 224
    INT %irq
    .set irq, irq + 1
.endr
