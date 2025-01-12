global dump_registers

[extern dump_registers_c]

dump_registers:
    ;save registers
    pusha
    push ds
    push es
    push fs
    push gs

    ; save address to return to
    mov eax, [esp + 36]
    push eax

    ; save error flags
    mov eax, [esp + 40]
    push eax
    call dump_registers_c
    add esp, 8

    ;restore registers
    pop gs
    pop fs
    pop es
    pop ds
    popa
    ret
