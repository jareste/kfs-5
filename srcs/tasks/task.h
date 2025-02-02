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
    uint32_t cpu_esp_;    // you might keep cpu state in a struct
    uint32_t pid;
    uintptr_t kernel_stack; // Kernel Stack (for syscalls)
    uintptr_t stack;        // User Stack
    struct task_struct *parent;
    struct task_struct *next;
    task_state_t state;
    char name[16];
    void (*on_exit)(void);
    void (*entry)(void);
    signal_context_t signals;
    size_t owner;
    void *mem_block;      // pointer to the big allocation
    size_t block_size;    // total size of the allocation
    int exit_status;
    uid_t uid;
    uid_t euid;
    gid_t gid;
    /* missing fields but untill it'll not work makes no sense to add them */    
} task_t;

void scheduler(void);
void start_foo_tasks(void);
void scheduler_init(void);
task_t* find_task(pid_t pid);
task_t* get_current_task();
void kill_task();

extern task_t* current_task;

#endif
