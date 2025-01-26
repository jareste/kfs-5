#ifndef TASK_H
#define TASK_H

#include "../keyboard/signals.h"
#include "../keyboard/idt.h"


typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t eip;
    uint32_t eflags;
    uint32_t cs;
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;
    uint32_t ss;
} CPUState;

typedef struct
{
    CPUState state;

    // Task metadata
    pid_t               pid;
    int                 alive_status;
    void*               stack;
    void*               heap;
    void*               code;
    void*               data;
    pid_t               parent_pid;
    pid_t*              children_pids;
    int                 uid;
    
    signal_context_t    signals;

    int*                file_descriptors;
    int                 priority;
    int                 time_slice; // in ms, means how long the task can run before being switched
} task_t;

void schedule();
void switch_to_task(task_t *next_task);
void init_tasks();
void print_task(task_t *task);

#endif // TASK_H
