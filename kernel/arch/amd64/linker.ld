ENTRY(kernel_entry)

PHDRS {
    load PT_LOAD;
}

SECTIONS {
    . = 5M;

    .text : ALIGN(4K) {
        k_interrupt_stubs_start = .;
        KEEP(*(.interrupt_stubs))
        k_interrupt_stubs_end = .;
        *(.text*)
    }

    .rodata : ALIGN(4K) {
        k_ctors_start = .;
        *(.init_array)
        k_ctors_end = .;
        *(.rodata*)
    }

    .data : ALIGN(4K) {
        *(.data*)
    }

    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss*)
    }

    /DISCARD/ : {
        *(.comment)
        *(.interp)
    }
}
