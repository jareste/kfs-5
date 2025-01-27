#include "task.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
#include "../utils/stdint.h"
#include "../display/display.h"
#include "../keyboard/signals.h"
#include "../gdt/gdt.h"

#define STACK_SIZE 4096

task_t *current_task = NULL;
task_t *task_list = NULL;
pid_t task_index = 0;

void kernel_main();
void task_1(void);

extern void switch_context(task_t *prev, task_t *next);

void scheduler(void)
{
    if (!current_task) return;
    
    task_t *next = current_task->next;
    // if (!next || next->state != TASK_READY)
    // {
    //     next = task_list;
    // }
    printf("Switching task %d -> %d\n", current_task->pid, next->pid);
    // printf("ESP: %p -> %p\n", current_task->cpu.esp_, next->cpu.esp_);
    // printf("EIP: %p -> %p\n", current_task->cpu.eip, next->cpu.eip);
    // printf("start_func: %p \n", task_1);
    // printf("current: %p -> %p\n", current_task, next);
    
    // if (next != current_task)
    // {
        task_t *prev = current_task;
        // printf("trying to switch context\n");
        // while(1);
        tss_set_stack(next->kernel_stack);
        // enable_interrupts();
    	// outb(0x20, 0x20);

        current_task = next;
        switch_context(prev, next);
        // while(1);
        puts("Switched\n");
    // }
    // printf("Task fault\n");
        // while(1);
}

void add_new_task(task_t* new_task)
{
    if (!task_list)
    {
        task_list = new_task;
        new_task->next = new_task;
    }
    else
    {
        task_t *current = task_list;
        while (current->next != task_list)
        {
            current = current->next;
        }
        current->next = new_task;
        new_task->next = task_list;
    }
}

void create_task(void (*entry)(void))
{
    task_t *task = kmalloc(sizeof(task_t));
    uint32_t *stack = kmalloc(STACK_SIZE);
    uint32_t *kernel_stack = kmalloc(STACK_SIZE);

    // Align stack and set up as if interrupted
    stack = (uint32_t*)((uint32_t)stack & 0xFFFFFFF0);
    stack += STACK_SIZE / sizeof(uint32_t);

    kernel_stack = (uint32_t*)((uint32_t)kernel_stack & 0xFFFFFFF0);
    kernel_stack += STACK_SIZE / sizeof(uint32_t);

    // Simulate interrupt frame (EIP, EFLAGS, etc.)
    // *--stack = 0x202;   // EFLAGS (IF enabled)
    // *--stack = 0x08;    // CS (kernel code segment)
    *--stack = (uint32_t)entry; // EIP

    task->pid = task_index++;
    task->cpu.esp_ = (uint32_t)stack; // Point to the simulated interrupt frame
    task->state = TASK_READY;
    task->kernel_stack = (uint32_t)kernel_stack;
    add_new_task(task);
    printf("Task %d created\n", task->pid);
}

void scheduler_init(void)
{
    task_t *idle = kmalloc(sizeof(task_t));
    uint32_t *stack = kmalloc(STACK_SIZE);
    stack += STACK_SIZE / sizeof(uint32_t);

    // *--stack = 0x202;
    // *--stack = 0x08;
    *--stack = (uint32_t)kernel_main;
    task_index = 0;

    idle->pid = task_index++;
    idle->cpu.esp_ = (uint32_t)stack;
    idle->cpu.eip = (uint32_t)kernel_main;
    idle->state = TASK_READY;
    idle->next = idle;
    current_task = idle;
    task_list = idle;
}

void task_1(void)
{
    puts("Task 1 Started\n");
    int i = 0;
    while (1)
    {
        i++;
        puts_color("Task 1\n", i %128);
        if (i % 1000 == 0)
        {
            scheduler();
        }
    }
}

void task_2(void)
{
    puts("Task 2 Started\n");
    int i = 0;
    while (1)
    {
        i++;
        puts("Task 2\n");
        if (i % 1000 == 0)
        {
            scheduler();
        }
    }
}

void start_foo_tasks(void)
{
    create_task(task_1);
    create_task(task_2);
}