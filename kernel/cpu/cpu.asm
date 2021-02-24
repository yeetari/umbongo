; void flush_gdt(Gdt *gdt)
global flush_gdt
flush_gdt:
    lgdt [rdi]
    mov rax, rsp
    push qword 0x10 ; ss
    push qword rax ; rsp
    pushfq ; rflags
    push qword 0x08 ; cs
    push qword .ret ; rip
    iretq
.ret:
    ; Load data selectors with null segments.
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret

extern syscall_handler
global syscall_stub
syscall_stub:
    swapgs
    mov [gs:16], rsp
    mov rsp, [gs:8]

    ; Push all GP registers.
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

    mov rdi, rsp
    call syscall_handler

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

    mov rsp, [gs:16]
    swapgs
    o64 sysret
