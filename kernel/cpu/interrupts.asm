%macro INT_ERR 1
    global int%1
    int%1:
        push qword %1
        jmp interrupt_stub
    .end:
        times ((int255.end - int255) - (int%1.end - int%1)) nop
%endmacro

%macro INT_NOERR 1
    global int%1
    int%1:
        ; Push dummy error code.
        push qword 0
        push qword %1
        jmp interrupt_stub
    .end:
        times ((int255.end - int255) - (int%1.end - int%1)) nop
%endmacro

extern interrupt_handler
interrupt_stub:
    ; Push all GP registers. No need to call cli/sti since setting DescriptorType::InterruptGate makes the CPU do that
    ; for us.
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Pass pointer to register file to allow modification in handler.
    mov rdi, rsp
    call interrupt_handler

    ; Pop back GP registers.
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Cleanup error code and interrupt number, then return from the interrupt.
    add rsp, 16
    iretq

section .interrupt_stubs

; Exceptions (AMD64 System Programming Manual revision 3.36 section 8.2)
INT_NOERR 0
INT_NOERR 1
INT_NOERR 2
INT_NOERR 3
INT_NOERR 4
INT_NOERR 5
INT_NOERR 6
INT_NOERR 7
INT_ERR 8
INT_NOERR 9
INT_ERR 10
INT_ERR 11
INT_ERR 12
INT_ERR 13
INT_ERR 14
INT_NOERR 15
INT_NOERR 16
INT_ERR 17
INT_NOERR 18
INT_NOERR 19
INT_NOERR 20
INT_ERR 21
INT_NOERR 22
INT_NOERR 23
INT_NOERR 24
INT_NOERR 25
INT_NOERR 26
INT_NOERR 27
INT_NOERR 28
INT_NOERR 29
INT_ERR 30
INT_NOERR 31

; IRQs.
%assign irq 32
%rep 224
    INT_NOERR irq
    %assign irq irq+1
%endrep
