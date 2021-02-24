ENTRY(kmain)

SECTIONS {
    . = 1M;

    .text : ALIGN(4K) {
        k_interrupt_stubs_start = .;
        KEEP(*(.interrupt_stubs))
        k_interrupt_stubs_end = .;
        *(.text*)

        . = ALIGN(4K);
        k_user_code_start = .;
        KEEP(*(.user_code*))
        k_user_code_end = .;
    }

    .rodata : ALIGN(4K) {
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
