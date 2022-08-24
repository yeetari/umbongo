%define AP_ABS(sym) sym - ap_bootstrap + 0x8000
%define AP_REL(sym) sym - ap_bootstrap

bits 16
global ap_bootstrap
global ap_bootstrap_end
extern ap_entry
ap_bootstrap:
    cli
    jmp 0x800:AP_REL(.set_cs) ; 0x800 << 4 == 0x8000
.set_cs:
    ; Set ds to cs so data accesses are offsetted from 0x8000 (through the AP_REL macro to avoid relocations).
    mov ax, cs
    mov ds, ax

    mov eax, [AP_REL(.cr4)]
    mov cr4, eax

    mov eax, [AP_REL(.cr3)]
    mov cr3, eax

    mov eax, [AP_REL(.efer)]
    mov ecx, 0xc0000080
    xor edx, edx
    wrmsr

    mov eax, [AP_REL(.cr0)]
    mov cr0, eax

    ; TODO: Remove protected mode jump step? AP_ABS(.long) isn't working for some reason.
    lgdt [AP_REL(.gdt32.ptr)]
    jmp 0x08:AP_ABS(.protected)

bits 32
.protected:
    lgdt [AP_REL(.gdt64.ptr)]
    lidt [AP_REL(.idt64.ptr)]
    jmp 0x08:.long

bits 64
.long:
    xor ecx, ecx
    mov eax, [AP_ABS(.xcr)]
    mov rdx, [AP_ABS(.xcr)]
    shr rdx, 32
    xsetbv

    mov eax, 1
    lock xadd [AP_ABS(.id)], eax
    xor rbp, rbp
    mov rsp, [AP_ABS(.stacks) + eax * 8]
    mov edi, eax
    inc edi ; BSP == 0
    call ap_entry

.cr0: dq 0
.cr3: dq 0
.cr4: dq 0
.xcr: dq 0
.efer: dq 0
.id: dd 0

align 4
.gdt32:
    dq 0x0
    dq 0x00cf9a000000ffff
    dq 0x00cf92000000ffff
.gdt32.end:
.gdt32.ptr:
    dw .gdt32.end - .gdt32 - 1
    dd AP_ABS(.gdt32)
.gdt64.ptr:
    dw 0
    dq 0
.idt64.ptr:
    dw 0
    dq 0
align 8
.stacks:
ap_bootstrap_end:

; uintptr_t *ap_prepare(void)
global ap_prepare
ap_prepare:
    mov rax, cr0
    mov [AP_ABS(ap_bootstrap.cr0)], rax
    mov rax, cr3
    mov [AP_ABS(ap_bootstrap.cr3)], rax
    mov rax, cr4
    mov [AP_ABS(ap_bootstrap.cr4)], rax

    xor ecx, ecx
    xgetbv
    or rax, rdx
    mov [AP_ABS(ap_bootstrap.xcr)], rax

    mov rcx, 0xc0000080
    rdmsr
    mov [AP_ABS(ap_bootstrap.efer)], rax

    sgdt [AP_ABS(ap_bootstrap.gdt64.ptr)]
    sidt [AP_ABS(ap_bootstrap.idt64.ptr)]
    mov rax, AP_ABS(ap_bootstrap.stacks)
    ret
