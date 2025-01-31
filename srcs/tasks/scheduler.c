#include "task.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
#include "../utils/stdint.h"
#include "../display/display.h"
#include "../keyboard/signals.h"
#include "../gdt/gdt.h"
#include "../kshell/kshell.h"

#define STACK_SIZE 4096
#define MAX_ACTIVE_TASKS 15

void kernel_main();
void task_1(void);
static void task_exit_pid(pid_t task_id);
static void task_exit_task(task_t* task);
extern void switch_context(task_t *prev, task_t *next);
void show_tasks();

static command_t commands[] = {
    {"show", "Show active tasks", show_tasks},
    {NULL, NULL, NULL}
};

task_t* current_task = NULL;
task_t* task_list = NULL;
task_t* to_free = NULL;
pid_t task_index = 0;

void free_finished_tasks()
{
    if (!to_free)
        return;
    kfree((void*)to_free->kernel_stack);
    kfree((void*)to_free->stack);
    kfree(to_free);
    to_free = NULL;
}

task_t* get_current_task()
{
    return current_task;
}

task_t* find_task(pid_t pid)
{
    task_t *current = task_list;
    do
    {
        if (current->pid == pid)
            return current;
        current = current->next;
    } while (current != task_list);
    return NULL;
}

void scheduler(void)
{
    if (!current_task) return;
    
    free_finished_tasks();

    task_t *next = current_task->next;

    if (next->pid == 0)
    {
        next = next->next;
    }

    task_t *prev = current_task;
    tss_set_stack(next->kernel_stack);

    current_task = next;
    uint32_t *stack = (uint32_t*)next->cpu.esp_;
    *--stack = (uint32_t)handle_signals;
    next->cpu.esp_ = (uint32_t)stack;
    switch_context(prev, next);
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

static void task_exit_task(task_t* task)
{
    if (task->on_exit)
        task->on_exit();

    task_t *prev = current_task;
    while (prev->next != task)
        prev = prev->next;

    prev->next = task->next;

    pid_t pid = task->pid;

    to_free = task;

    scheduler();
}

static void task_exit_pid(pid_t task_id)
{
    task_t *task = find_task(task_id);

    task_exit_task(task);
}

/* Cb function to be called once a task returns
*/
static void task_exit()
{
    task_exit_task(current_task);
}

void kill_task()
{
    task_exit_task(current_task);
    // current_task->state = TASK_TO_DIE;
}

void create_task(void (*entry)(void), char* name, void (*on_exit)(void))
{
    task_t *task = kmalloc(sizeof(task_t));
    uint32_t *stack = kmalloc(STACK_SIZE);
    uint32_t *kernel_stack = kmalloc(STACK_SIZE);
    task->stack = (uint32_t)stack; /* Point to the stack for being able to release it. */
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
    task->on_exit = on_exit;
    init_signals(task);
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
    to_free = NULL;
    install_all_cmds(commands, TASKS);
}

/*************************************** */

void task_write(void)
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

        // puts_color("TEST_WRITE\n", LIGHT_MAGENTA);
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

void task_2_exit()
{
    puts_color("Task 2 exited\n", RED);
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

void recursion()
{
    static unsigned int i = 0;
    // puts("Recursion\n" );
    printf("Recursion %d\n", i++);
    scheduler();
    recursion();
}

void test_recursion(void)
{
    recursion();
}

void task_read()
{
    while (1)
    {
        char buffer[10];
        int return_value;
        // puts_color("TEST_READ\n", LIGHT_MAGENTA);
        // printf("Buffer before sys_read: %s\n", buffer);

        asm volatile (
            "mov $2, %%eax\n"
            "mov $0, %%ebx\n"
            "mov %1, %%ecx\n"
            "mov %2, %%edx\n"
            "int $0x80\n"
            "mov %%eax, %0\n"
            : "=r"(return_value)
            : "r"(buffer), "r"(sizeof(buffer))
            : "eax", "ebx", "ecx", "edx"
        );

        // printf("Buffer after sys_read: '");
        // for (size_t i = 0; i < 10; i++)
        // {
        //     if (buffer[i] == 0)
        //         break;
        //     putc(buffer[i]);
        // }
        // puts("'\n");
        // printf("SYS_READ return value: %d\n", return_value);
        scheduler();
    }
}

void kshell();
void start_foo_tasks(void)
{
    create_task(task_1, "task_1", NULL);
    create_task(task_2, "task_2", task_2_exit);
    // create_task(task_write, "task_3");
    create_task(task_read, "task_read", NULL);
    create_task(kshell, "kshell", NULL);
    // create_task(test_recursion, "recursion");
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
