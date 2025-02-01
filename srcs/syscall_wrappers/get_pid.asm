%define syscall int 0x80

global get_pid
get_pid:
    push ebp
    mov ebp, esp

    mov eax, 5

    syscall

    pop ebp
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
