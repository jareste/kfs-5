#include "task.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
#include "../utils/stdint.h"
#include "../display/display.h"
#include "../keyboard/signals.h"

#define STACK_SIZE 4096

task_t *current_task = NULL;
task_t *task_list = NULL;
pid_t task_index = 0;

void kernel_main();

extern void switch_context(task_t *prev, task_t *next);

void scheduler(void)
{
    if (!current_task) return;
    
    task_t *next = current_task->next;
    if (!next || next->state != TASK_READY)
    {
        next = task_list;
    }
    printf("Switching task %d -> %d\n", current_task->pid, next->pid);
    printf("ESP: %p -> %p\n", current_task->cpu.esp_, next->cpu.esp_);
    printf("EIP: %p -> %p\n", current_task->cpu.eip, next->cpu.eip);
    printf("current: %p -> %p\n", current_task, next);
    
    if (next != current_task)
    {
        task_t *prev = current_task;
        current_task = next;
        switch_context(prev, current_task);
        puts("Switched\n");
        while(1);
    }
}

void create_task(void (*entry)(void))
{
    task_t *task = kmalloc(sizeof(task_t));
    uint32_t *stack = kmalloc(STACK_SIZE);

    stack = (uint32_t*)((uint32_t)stack & 0xFFFFFFF0);
    stack += STACK_SIZE / sizeof(uint32_t);

    *--stack = (uint32_t) entry;

    task->cpu.esp_ = (uint32_t) stack;
    task->cpu.eip = (uint32_t) entry;
    task->cpu.eflags = 0x202;
    // task->ebx = 0;
    // task->ecx = 0;
    // task->edx = 0;
    // task->esi = 0;
    // task->edi = 0;
    // task->ebp = 0;
    // task->esp = (uint32_t)stack;
    // task->state = TASK_READY;
    // task->next = task_list;
    // task_list = task;
    // task->eip = (uint32_t)entry;
    // task->eflags = 0x202;
}

void scheduler_init(void)
{
    task_t *idle = kmalloc(sizeof(task_t));
    uint32_t *stack = kmalloc(STACK_SIZE);
    stack += STACK_SIZE / sizeof(uint32_t);

    // *--stack = 0x202;
    // *--stack = 0x08;
    *--stack = (uint32_t)kernel_main;

    idle->cpu.esp_ = (uint32_t)stack;
    idle->cpu.eip = (uint32_t)kernel_main;
    idle->state = TASK_RUNNING;
    idle->next = idle;
    current_task = idle;
    task_list = idle;
}

void task_1(void)
{
    puts("Task 1 Started\n");
    while (1)
    {
        puts("Task 1\n");
    }
}

void task_2(void)
{
    puts("Task 2 Started\n");
    while (1)
    {
        puts("Task 2\n");
    }
}

void start_foo_tasks(void)
{
    create_task(task_1);
    create_task(task_2);
}