BITS 32

section .multiboot
align 4
    dd 0x1BADB002   ; Multiboot magic number
    dd 0x00000003   ; Flags: align modules on page boundaries and provide memory map
    dd -(0x1BADB002 + 0x00000003) ; Checksum

section .text
global start
start:
    cli                     ; Disable interrupts
    mov esp, 0x90000        ; Set up stack
    mov eax, 0x100000       ; Kernel load address
    jmp eax                 ; Jump to kernel entry point

section .note.GNU-stack noalloc noexec nowrite progbits

TIMES 510 - ($ - $$) db 0  ; Pad to 510 bytes
dw 0xAA55                  ; Boot sector signature
