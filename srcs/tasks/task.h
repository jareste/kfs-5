#ifndef TASK_H
#define TASK_H

#include "../keyboard/signals.h"
#include "../keyboard/idt.h"

typedef struct
{
    int                 pid;
    int                 state;
    void*               stack;
    void*               heap;
    void*               code;
    void*               data;
    int                 parent_pid;
    pid_t*              children_pids;
    int                 uid;
    
    signal_context_t    signals;

    int*                file_descriptors;
    int                 priority;
    int                 time_slice; // in ms, means how long the task can run before being switched
    
    // uint32_t eax, ebx, ecx, edx, esi, edi, esp, ebp, eip, eflags;
    registers           regs;
    error_state         err;
} task_t;

void schedule();

#endif // TASK_H
