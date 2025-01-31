#ifndef TASK_H
#define TASK_H

#include "../utils/stdint.h"
#include "../keyboard/signals.h"
#include "cpu_state.h"
typedef enum
{
    TASK_RUNNING,
    TASK_READY,
    TASK_ZOMBIE,
    TASK_WAITING,
    TASK_TO_DIE
} task_state_t;

typedef struct task_struct
{
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
    uintptr_t kernel_stack; // Kernel Stack (for syscalls)
    uintptr_t stack;        // User Stack
    char name[16];
    void (*on_exit)(void);
    signal_context_t signals;
    /* missing fields but untill it'll not work makes no sense to add them */    
} task_t;

void scheduler(void);
void start_foo_tasks(void);
void scheduler_init(void);
task_t* find_task(pid_t pid);
task_t* get_current_task();
void kill_task();

#endif
