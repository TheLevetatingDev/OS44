ENTRY(kernel_main)

SECTIONS {
    . = 0x100000;  /* KEEP AT 1MB physical — clearly safe from your debug log */

    .text ALIGN(4096) : {
        *(.text*)
    }

    .rodata ALIGN(4096) : {
        *(.rodata*)
    }

    .data ALIGN(4096) : {
        *(.data*)
    }

    .bss ALIGN(4096) : {
        *(COMMON)
        *(.bss*)
    }
}