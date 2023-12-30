.file "smp.S"
.text

#define AP_BASE 0x8000
#define AP_ABS(symbol) (symbol - ap_trampoline + AP_BASE)

.extern ap_entry
.globl ap_trampoline
.globl ap_trampoline_end
.type ap_trampoline, "function"

.code16
ap_trampoline:
    cli
    cld
    jmp $0x0, $AP_ABS(1f)

1:
    // Zero all segment selectors.
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    // Enable PAE.
    mov %cr4, %eax
    or $(1 << 5), %eax
    mov %eax, %cr4

    // Load identity-mapped kernel PML4.
    mov AP_ABS(.cr3), %eax
    mov %eax, %cr3

    // Enable long mode (EFER.LME).
    mov $0xc0000080, %ecx
    rdmsr
    or $(1 << 8), %eax
    wrmsr

    // Enable protection and paging.
    mov %cr0, %eax
    or $(1 << 31 | 1), %eax
    mov %eax, %cr0

    // Far jump to protected mode.
    lgdt AP_ABS(.gdt32.ptr)
    jmp $0x8, $AP_ABS(.Lprotected)

.code32
.Lprotected:
    // Far jump to long mode.
    lgdt AP_ABS(.gdt64.ptr)
    jmp $0x8, $.Llong

.code64
.Llong:
    // Load stack pointer.
    xor %rbp, %rbp
    mov AP_ABS(.stack), %rsp

    // Call into C++.
    call ap_entry

.align 4
.cr3: .zero 8
.stack: .zero 8

.align 8
.gdt32:
    .8byte 0x0
    .8byte 0x00cf9a000000ffff
    .8byte 0x00cf92000000ffff
.gdt32.ptr:
    .2byte . - .gdt32 - 1
    .4byte AP_ABS(.gdt32)

.align 8
.gdt64:
    .8byte 0x0
    .8byte 0x00209a0000000000
    .8byte 0x0000920000000000
.gdt64.ptr:
    .2byte . - .gdt64 - 1
    .4byte AP_ABS(.gdt64)

ap_trampoline_end:

.globl smp_prepare
.type smp_prepare, "function"
smp_prepare:
    // Store kernel PML4.
    mov %cr3, %rax
    mov %rax, AP_ABS(.cr3)

    // Return a pointer to the stack pointer.
    mov $AP_ABS(.stack), %rax
    ret
