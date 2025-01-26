global save_cpu_state
global restore_cpu_state

; void save_cpu_state(CPUState *state);
save_cpu_state:
    mov eax, [esp + 4]

    mov [eax + 0], ebx
    mov [eax + 4], ecx
    mov [eax + 8], edx
    mov [eax + 12], esi
    mov [eax + 16], edi
    mov [eax + 20], ebp
    mov [eax + 24], esp

    mov ebx, [esp]
    mov [eax + 28], ebx

    pushf
    pop ebx
    mov [eax + 32], ebx

    mov [eax + 36], cs
    mov [eax + 40], ds
    mov [eax + 44], es
    mov [eax + 48], fs
    mov [eax + 52], gs
    mov [eax + 56], ss

    ret

; void restore_cpu_state(CPUState *state);
restore_cpu_state:
    mov eax, [esp + 4]

    mov ss, [eax + 56]
    mov gs, [eax + 52]
    mov fs, [eax + 48]
    mov es, [eax + 44]
    mov ds, [eax + 40]
    mov cs, [eax + 36]

    push dword [eax + 32]
    popf

    mov ebx, [eax + 0]
    mov ecx, [eax + 4]
    mov edx, [eax + 8]
    mov esi, [eax + 12]
    mov edi, [eax + 16]
    mov ebp, [eax + 20]
    mov esp, [eax + 24]

    push dword [eax + 28]
    ret
