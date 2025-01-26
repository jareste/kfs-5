ENTRY(start)

SECTIONS {
    . = 0x100000;
    
    .multiboot : { *(.multiboot) }

    .text : { *(.text) }

    .rodata : { *(.rodata*) }

    .data ALIGN (0x1000) : { *(.data) }

    .bss : {
        *(.bss)
        *(COMMON)
        ebss = .;
        endkernel = .;
    }
}
