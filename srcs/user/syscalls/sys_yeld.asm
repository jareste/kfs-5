%define syscall int 0x30

global yeld
yeld:
    push ebp
    mov ebp, esp

    mov eax, 158

    syscall

    pop ebp
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
