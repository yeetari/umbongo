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
    ret

global syscall_stub
syscall_stub:
    jmp $
