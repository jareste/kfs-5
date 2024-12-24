BITS 32

section .text
start:
    cli                 ; Disable interrupts
    mov esp, 0x90000    ; Set up stack
    mov eax, 0x100000   ; Kernel load address
    jmp eax             ; Jump to kernel entry point

TIMES 510 - ($ - $$) db 0 ; Pad to 510 bytes
dw 0xAA55                ; Boot sector signature

