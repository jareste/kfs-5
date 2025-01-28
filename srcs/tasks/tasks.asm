[bits 32]
global switch_context

switch_context:
    ; Save old task's state from the stack
    mov eax, [esp + 4]   ; old_task pointer

    ; Save pusha registers into old_task
    mov ecx, [esp + 24]  ; EBX from pusha data
    mov [eax + 0], ecx   ; old_task->ebx
    mov ecx, [esp + 16]  ; ECX
    mov [eax + 4], ecx   ; old_task->ecx
    mov ecx, [esp + 20]  ; EDX
    mov [eax + 8], ecx   ; old_task->edx
    mov ecx, [esp + 36]  ; ESI
    mov [eax + 12], ecx  ; old_task->esi
    mov ecx, [esp + 40]  ; EDI
    mov [eax + 16], ecx  ; old_task->edi
    mov ecx, [esp + 32]  ; EBP
    mov [eax + 20], ecx  ; old_task->ebp
    mov ecx, [esp + 28]  ; Original ESP (from pusha)
    mov [eax + 24], ecx  ; old_task->esp_

    ; Save EIP and EFLAGS from interrupt frame
    mov ecx, [esp + 52]  ; EIP
    mov [eax + 28], ecx  ; old_task->eip
    mov ecx, [esp + 44]  ; EFLAGS
    mov [eax + 32], ecx  ; old_task->eflags

    ; Restore new task's state to the stack
    mov eax, [esp + 8]   ; new_task pointer

    ; Restore pusha registers from new_task
    mov ecx, [eax + 0]   ; new_task->ebx
    mov [esp + 24], ecx
    mov ecx, [eax + 4]   ; new_task->ecx
    mov [esp + 16], ecx
    mov ecx, [eax + 8]   ; new_task->edx
    mov [esp + 20], ecx
    mov ecx, [eax + 12]  ; new_task->esi
    mov [esp + 36], ecx
    mov ecx, [eax + 16]  ; new_task->edi
    mov [esp + 40], ecx
    mov ecx, [eax + 20]  ; new_task->ebp
    mov [esp + 32], ecx
    mov ecx, [eax + 24]  ; new_task->esp_
    mov [esp + 28], ecx  ; Original ESP in pusha data

    ; Restore EIP and EFLAGS from new_task
    mov ecx, [eax + 28]  ; new_task->eip
    mov [esp + 52], ecx  ; Update EIP in interrupt frame
    mov ecx, [eax + 32]  ; new_task->eflags
    mov [esp + 44], ecx  ; Update EFLAGS in interrupt frame

    ; Return to the interrupt handler, which will then popa and iret with the new task's state
    ret

global switch_context_no_int
switch_context_no_int:
    ; Save old task's registers
    mov eax, [esp + 4]   ; old_task pointer
    mov [eax + 0],  ebx
    mov [eax + 4],  ecx
    mov [eax + 8],  edx
    mov [eax + 12], esi
    mov [eax + 16], edi
    mov [eax + 20], ebp
    mov [eax + 24], esp
    ; pushfd                 ; get the old EFLAGS into stack
    ; pop dword [eax + 28]        ; store them in old_task->cpu.eflags (for example)

    ; Restore new task's registers
    mov eax, [esp + 8]   ; new_task pointer
    mov ebx, [eax + 0]
    mov ecx, [eax + 4]
    mov edx, [eax + 8]
    mov esi, [eax + 12]
    mov edi, [eax + 16]
    mov ebp, [eax + 20]
    mov esp, [eax + 24]
    ; push dword [eax + 28]        ; restore new_task->cpu.eflags
    ; popfd

    ; .switch_to_new_task:
    ; jmp .switch_to_new_task

    ; Return to new task's EIP (stored on its stack)
    ret

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