ENTRY(kmain)

PHDRS {
    load PT_LOAD;
}

SECTIONS {
    . = 1M;

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
