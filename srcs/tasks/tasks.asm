[bits 32]
global switch_context

switch_context:
    ; Save old task's registers
    mov eax, [esp + 4]   ; old_task pointer
    mov [eax + 0],  ebx
    mov [eax + 4],  ecx
    mov [eax + 8],  edx
    mov [eax + 12], esi
    mov [eax + 16], edi
    mov [eax + 20], ebp
    mov [eax + 24], esp
    pushfd                 ; get the old EFLAGS into stack
    pop dword [eax + 28]        ; store them in old_task->cpu.eflags (for example)

    ; Restore new task's registers
    mov eax, [esp + 8]   ; new_task pointer
    mov ebx, [eax + 0]
    mov ecx, [eax + 4]
    mov edx, [eax + 8]  
    mov esi, [eax + 12]
    mov edi, [eax + 16]
    mov ebp, [eax + 20]
    mov esp, [eax + 24]
    push dword [eax + 28]        ; restore new_task->cpu.eflags
    popfd

    ; .switch_to_new_task:
    ; jmp .switch_to_new_task

    ; Return to new task's EIP (stored on its stack)
    ret

[bits 32]
global fork_trampoline
fork_trampoline:

    ; Set the return value for fork in the child to 0.
    mov eax, 0
    ; Now, return to the original return address.
    ; .kkk:
    ; jmp .kkk
    ret

global capture_cpu_state
capture_cpu_state:
    ; Save old task's registers
    mov eax, [esp + 4]   ; old_task pointer
    mov [eax + 0],  ebx
    mov [eax + 4],  ecx
    mov [eax + 8],  edx
    mov [eax + 12], esi
    mov [eax + 16], edi
    mov [eax + 20], ebp
    mov [eax + 24], esp
    pushfd                 ; get the old EFLAGS into stack
    pop dword [eax + 28]        ; store them in old_task->cpu.eflags (for example)

    ; Return to new task's EIP (stored on its stack)
    ret





section .note.GNU-stack noalloc noexec nowrite progbits


global copy_context

copy_context:
    ; Copy old task's registers to new task's registers
    mov eax, [esp + 4]   ; old_task pointer
    mov ebx, [esp + 8]   ; new_task pointer

    mov ecx, [eax + 0]   ; Copy EBX
    mov [ebx + 0], ecx

    mov ecx, [eax + 4]   ; Copy ECX
    mov [ebx + 4], ecx

    mov ecx, [eax + 8]   ; Copy EDX
    mov [ebx + 8], ecx

    mov ecx, [eax + 12]  ; Copy ESI
    mov [ebx + 12], ecx

    mov ecx, [eax + 16]  ; Copy EDI
    mov [ebx + 16], ecx

    mov ecx, [eax + 20]  ; Copy EBP
    mov [ebx + 20], ecx

    mov ecx, [eax + 24]  ; Copy ESP
    mov [ebx + 24], ecx

    ; Uncomment if you need to copy EFLAGS
    ; mov ecx, [eax + 28]  ; Copy EFLAGS
    ; mov [ebx + 28], ecx

    ret

section .note.GNU-stack noalloc noexec nowrite progbits

section .note.GNU-stack noalloc noexec nowrite progbits

; global switch_context
; switch_context:
;     mov eax, [esp + 4]  ; prev task
;     mov [eax], esp       ; Save current ESP to prev->cpu.esp_
;     mov eax, [esp + 8]  ; next task
;     mov esp, [eax]       ; Load next task's ESP
;     ret                  ; Return to handler, which will iret to the new task

; switch_context:
;     ; Save old task's state
;     mov eax, [esp + 4]   ; old_task pointer
;     mov [eax + 0],  ebx
;     mov [eax + 4],  ecx
;     mov [eax + 8],  edx
;     mov [eax + 12], esi
;     mov [eax + 16], edi
;     mov [eax + 20], ebp
;     mov [eax + 24], esp
;     ; pushfd                ; Save EFLAGS
;     ; pop dword [eax + 28]  ; Store in task_struct


;     ; Restore new task's state

;     mov eax, [esp + 8]   ; new_task pointer
;     mov ebx, [eax + 0]
;     mov ecx, [eax + 4]
;     mov edx, [eax + 8]
;     mov esi, [eax + 12]
;     mov edi, [eax + 16]
;     mov ebp, [eax + 20]
;     mov esp, [eax + 24]
;     ; push dword [eax + 28] ; Restore EFLAGS
;     ; popfd

;     iret