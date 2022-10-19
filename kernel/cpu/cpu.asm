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
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret

; void switch_now(RegisterState *regs)
global switch_now
switch_now:
    push qword [rdi + 168] ; ss
    push qword [rdi + 160] ; rsp
    push qword [rdi + 152] ; rflags
    push qword [rdi + 144] ; cs
    push qword [rdi + 136] ; rip
    mov r15, [rdi + 0]
    mov r14, [rdi + 8]
    mov r13, [rdi + 16]
    mov r12, [rdi + 24]
    mov r11, [rdi + 32]
    mov r10, [rdi + 40]
    mov r9, [rdi + 48]
    mov r8, [rdi + 56]
    mov rbp, [rdi + 64]
    mov rsi, [rdi + 72]
    mov rdx, [rdi + 88]
    mov rcx, [rdi + 96]
    mov rbx, [rdi + 104]
    mov rax, [rdi + 112]
    mov rdi, [rdi + 80]
    iretq

extern syscall_handler
global syscall_stub
syscall_stub:
    mov [gs:16], rsp
    mov rsp, [gs:8]
    push qword [gs:16]

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
    mov rsi, [gs:24]
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

    pop rsp
    o64 sysret

section .user_access

; TODO: More optimal version for CPUs not supporting FSRM (anything pre ice lake or zen 3).
; bool user_copy(uintptr_t dst, uintptr_t src, uint32_t size)
global user_copy
user_copy:
    mov ecx, edx ; copy size into count register
    xor eax, eax
    rep movsb
    ret
global user_copy_fault
user_copy_fault:
    inc eax
    ret
