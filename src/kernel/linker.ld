ENTRY(kernel_main)

SECTIONS {
    . = 0x200000;  /* 2MB — avoids UEFI reserved low memory around 1MB */

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