#include "task.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
#include "../utils/stdint.h"
#include "../display/display.h"
#include "../keyboard/signals.h"
#include "../gdt/gdt.h"
#include "../kshell/kshell.h"

#define STACK_SIZE 4096

void kernel_main();
void task_1(void);

extern void switch_context(task_t *prev, task_t *next);
void show_tasks();


static command_t commands[] = {
    {"stasks", "Show active tasks", show_tasks},
    {NULL, NULL, NULL}
};

task_t *current_task = NULL;
task_t *task_list = NULL;
pid_t task_index = 0;

void scheduler(void)
{
    if (!current_task) return;
    
    task_t *next = current_task->next;
    // if (!next || next->state != TASK_READY)
    // {
    //     next = task_list;
    // }
    // printf("Switching task %d -> %d\n", current_task->pid, next->pid);
    // printf("ESP: %p -> %p\n", current_task->cpu.esp_, next->cpu.esp_);
    // printf("EIP: %p -> %p\n", current_task->cpu.eip, next->cpu.eip);
    // printf("start_func: %p \n", task_1);
    // printf("current: %p -> %p\n", current_task, next);
    
    if (next->pid == 0)
    {
        next = next->next;
    }

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
        // puts("Switched\n");
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

void task_exit()
{
    current_task->state = 3;

    // Remove from task list
    printf("Task %d exited\n", current_task->pid);
    task_t *prev = task_list;
    while (prev->next != current_task)
        prev = prev->next;

    prev->next = current_task->next; // Skip current task

    kfree((void*)current_task->kernel_stack);
    kfree(current_task);

    scheduler(); // Switch to another task
}


void create_task(void (*entry)(void), char* name)
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
    *--stack = (uint32_t)task_exit; // EIP
    *--stack = (uint32_t)entry; // EIP

    task->pid = task_index++;
    task->cpu.esp_ = (uint32_t)stack; // Point to the simulated interrupt frame
    task->state = TASK_READY;
    task->kernel_stack = (uint32_t)kernel_stack;
    memcpy(task->name, name, strlen(name) > 15 ? 15 : strlen(name));
    task->name[strlen(name) > 15 ? 15 : strlen(name)] = '\0';
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
    memcpy(idle->name, "idle", 4);
    idle->name[4] = '\0';
    current_task = idle;
    task_list = idle;
    install_all_cmds(commands, TASKS);
}

void task_3(void)
{
    puts("Task 3 Started\n");
    int i = 0;
    int return_value;
    while (1)
    {
        i++;
        // puts_color("Task 3\n", RED);
        // if (i % 1000 == 0)
        // {

        const char* msg = "Hello, world!\n";
        size_t msg_len = strlen(msg);

        puts_color("TEST_WRITE\n", LIGHT_MAGENTA);
        asm volatile (
            "mov $1, %%eax\n"
            "mov $1, %%ebx\n"
            "mov %1, %%ecx\n"
            "mov %2, %%edx\n"
            "int $0x80\n"
            "mov %%eax, %0\n"
            : "=r"(return_value)
            : "r"(msg), "r"(msg_len)
            : "eax", "ebx", "ecx", "edx"
        );
        // printf("SYS_WRITE return value: %d\n", return_value);

            scheduler();
        // }
    }
}

void task_1(void)
{
    puts("Task 1 Started\n");
    int i = 0;
    while (1)
    {
        i++;
        // puts_color("Task 1\n", i %128);
        if (i == 1000)
        {
            // create_task(task_3);
        }
        // if (i % 1000 == 0)
        // {
            scheduler();
        // }
    }
}

void task_2(void)
{
    puts("Task 2 Started\n");
    int i = 0;
    i = 0;
    while (i < 500)
    {
        i++;
        // puts("Task 2\n");
        // if (i % 1000 == 0)
        // {
            scheduler();
        // }
    }
}

void kshell();
void start_foo_tasks(void)
{
    create_task(task_1, "task_1");
    create_task(task_2, "task_2");
    create_task(kshell, "kshell");
}

/* ################################################################### */
/*                               TESTS                                 */
/* ################################################################### */
void show_tasks()
{
    task_t *current = task_list;
    do
    {
        printf("Task '%s'\n", current->name);
        printf("  PID: %d\n", current->pid);
        printf("  ESP: %p\n", current->cpu.esp_);
        printf("  EIP: %p\n", current->cpu.eip);
        printf("  State: %d\n", current->state);
        current = current->next;
    } while (current != task_list);
}
