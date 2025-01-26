#include "task.h"
#include "../display/display.h"
#include "../memory/memory.h"

#define MAX_TASKS 2
#define STACK_SIZE 4096

// uint8_t task1_stack[STACK_SIZE];
// uint8_t task2_stack[STACK_SIZE];

task_t tasks[2];

task_t *current_task = &tasks[0];
task_t *task_list[] = {&tasks[0], &tasks[1]};
int current_task_index = 0;

typedef enum
{
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
} task_state;

extern void save_cpu_state(CPUState *state);
extern void restore_cpu_state(CPUState *state);

void print_task(task_t *task)
{
    printf("Task %d:\n", task->pid);
    printf("  EIP: %p\n", task->state.eip);
    printf("  ESP: %x\n", task->state.esp);
    printf("  EFLAGS: %x\n", task->state.eflags);
    printf("  Stack: %p\n", task->stack);
}


void switch_to_task(task_t *next_task) {
    disable_interrupts();
    save_cpu_state(&current_task->state);
    printf("Switching to task %d\n", next_task->pid);
    restore_cpu_state(&next_task->state);
    current_task = next_task;
    enable_interrupts();
}

void test_context_switch()
{
    printf("Testing context switch...\n");
    print_task(&tasks[0]);
    // save_cpu_state(&tasks[0]); // Save state of task 0
    save_cpu_state(&tasks[0].state);
    printf("Saved task0...\n");
    print_task(&tasks[0]);
    restore_cpu_state(&tasks[1].state); // Restore state of task 1
    // load_state(&tasks[0]); // Immediately restore state of task 0
    printf("Restored task0...\n");
}


void schedule()
{
    // current_task_index = (current_task_index + 1) % MAX_TASKS;
    current_task_index++;
    if (current_task_index >= MAX_TASKS)
    {
        current_task_index = 0;
    }
    printf("Scheduling... %d\n", current_task_index);
    task_t *next_task = task_list[current_task_index];
    disable_interrupts();
    switch_to_task(next_task);
    // test_context_switch();
    enable_interrupts();

    // switch_to_task(task_list[current_task_index]);
    current_task = next_task;
}

// void init_task(task_t *task, void (*entry)(), uint8_t *stack, int pid)
// {
//     task->pid = pid;
//     task->state = READY;
//     task->stack = stack + STACK_SIZE - 16;
//     task->eip = (uint32_t)entry;
//     task->esp = (uint32_t)(stack + STACK_SIZE - 16);
//     task->eflags = 0x202;
//     // printf("Offset of edi: %z\n", offsetof(task_t, edi));
//     // printf("Offset of eip: %z\n", offsetof(task_t, eip));
//     print_task(task);
// }

void init_task(task_t *task, void (*entry)(), size_t stack_size, int pid)
{
    task->pid = pid;
    task->alive_status = READY;

    uint8_t *stack = (uint8_t *)vmalloc(stack_size, false);
    if (!stack)
    {
        printf("Error: Failed to allocate stack for task %d\n", pid);
        return;
    }

    uint32_t *stack_ptr = (uint32_t *)(stack + stack_size);
    stack_ptr = (uint32_t *)((uint32_t)stack_ptr & ~0xF);

    task->stack = (void *)stack;
    task->state.esp = (uint32_t)stack_ptr;
    task->state.eip = (uint32_t)entry;
    task->state.eflags = 0x202;

    task->state.edi = 0;
    task->state.esi = 0;
    task->state.ebp = 0;
    task->state.ebx = 0;
    task->state.edx = 0;
    task->state.ecx = 0;
    task->state.eax = 0;

    printf("Initialized task %d:\n", pid);
    printf("  Entry: %p\n", entry);
    printf("  ESP: %x\n", task->state.esp);
    printf("  EIP: %x\n", task->state.eip);
}


const char* debug_msg = "Debug message";
void print_debug(const char *message, uint32_t value)
{
    printf("%s: %p\n", message, value);
}


void task1()
{
    printf("Task 1 running\n");
    while (1) asm("hlt");
}

void task2()
{
    printf("Task 2 running\n");
    while (1) asm("hlt");
}

void init_tasks()
{
    init_task(&tasks[0], task1, KB(4), 1);
    init_task(&tasks[1], task2, KB(4), 2);
    // switch_to_task(&tasks[0]);  // Start first task
    enable_interrupts();

    task1();
}
