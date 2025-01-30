%define syscall int 0x80

global write
write:
    push ebp
    mov ebp, esp

    mov ebx, [ebp + 8]
    mov ecx, [ebp + 12]
    mov edx, [ebp + 16]
    mov eax, 1

    syscall

    pop ebp
    ret
