global gdt_flush

gdt_flush:
    mov ax, 0x10        ; Kernel data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov ax, 0x18        ; Kernel stack segment selector
    mov ss, ax

    jmp 0x08:reload_cs  ; Jump to the next instruction, far jump

reload_cs:
    ret                 ; Return to kernel_main

section .note.GNU-stack noalloc noexec nowrite progbits
