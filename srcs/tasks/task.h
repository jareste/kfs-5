#ifndef TASK_H
#define TASK_H

#include "../utils/stdint.h"
#include "cpu_state.h"
typedef enum
{
    TASK_RUNNING,
    TASK_READY,
    TASK_ZOMBIE
} task_state_t;

typedef struct task_struct {
    cpu_state_t cpu;
    uint32_t pid;
    task_state_t state;
    struct task_struct *parent;
    struct task_struct *next;
    // uint32_t esp;       // Stack Pointer
    // uint32_t eip;       // Instruction Pointer
    // uint32_t ebx, ecx, edx, esi, edi, ebp;
    // uint32_t eax;
    // uint32_t eflags;
    uint32_t kernel_stack; // Kernel Stack (for syscalls)
    /* missing fields but untill it'll not work makes no sense to add them */    
} task_t;

extern task_t *current_task;

void scheduler(void);
void start_foo_tasks(void);
void scheduler_init(void);


#endif
