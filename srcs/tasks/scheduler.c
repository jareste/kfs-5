#include "task.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
#include "../utils/stdint.h"
#include "../display/display.h"
#include "../keyboard/signals.h"
#include "../gdt/gdt.h"
#include "../kshell/kshell.h"
#include "../syscall_wrappers/stdlib.h"

#define STACK_SIZE 4096
#define MAX_ACTIVE_TASKS 15

void kernel_main();
void task_1(void);
void task_1_exit();
static void task_exit_pid(pid_t task_id);
static void task_exit_task(task_t* task);
extern void switch_context(task_t *prev, task_t *next);
extern void copy_context(task_t *prev, task_t *next);
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
    if (next->state == TASK_READY)
    {
        next->state = TASK_RUNNING;
    }
    else if (next->state == TASK_RUNNING)
    {
        uint32_t *stack = (uint32_t*)next->cpu.esp_;
        *--stack = (uint32_t)handle_signals;
        next->cpu.esp_ = (uint32_t)stack;
    }
    // if (next->pid == 5)
    //     while(1);
    
    // printf("Current task: %d, %p\n", current_task->pid, next);
    // puts_color("Scheduler\n", LIGHT_MAGENTA);
    switch_context(prev, next);
    // puts_color("Scheduler\n", RED);
    /* think this should not be here maybe (?)*/
    enable_interrupts();
    outb(0x20, 0x20);
}

void add_new_task(task_t* new_task)
{
    if (!task_list)
    {
        task_list = new_task;
        new_task->next = new_task;
        current_task = task_list;
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
    printf("Task %d added\n", new_task->pid);
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
    task_t *task;
    uint32_t *stack;
    uint32_t *kernel_stack;

    if (task_index >= MAX_ACTIVE_TASKS)
    {
        puts_color("Max number of tasks reached\n", RED);
        return;
    }
    
    task = kmalloc(sizeof(task_t));
    stack = kmalloc(STACK_SIZE);
    kernel_stack = kmalloc(STACK_SIZE);
    
    memset(stack, 0xFF, STACK_SIZE);
    task->stack = (uint32_t)stack; /* Point to the stack for being able to release it. */

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
    task->entry = entry;
    init_signals(task);
    add_new_task(task);
    printf("Task %d created\n", task->pid);
}

int _fork(void)
{
    if (task_index >= MAX_ACTIVE_TASKS)
    {
        puts_color("Max number of tasks reached\n", RED);
        return -1;
    }
    task_t *parent = current_task;
    
    task_t *child = kmalloc(sizeof(task_t));
    if (!child)
        return -1;  // handle allocation failure as needed


    uint32_t *child_stack_alloc = kmalloc(STACK_SIZE);
    if (!child_stack_alloc)
        return -1;  // error handling

    /* Copy the entire stack memory.  
     * (Using a scan for 0xFFFFFFFF isn’t safe since valid data might equal that.)
     */
    // memcpy(child_stack_alloc, parent_stack_alloc, STACK_SIZE);
    memset(child_stack_alloc, 0xFF, STACK_SIZE);


    child->stack = (uint32_t)child_stack_alloc;
    child_stack_alloc = (uint32_t*)((uint32_t)child_stack_alloc & 0xFFFFFFF0);
    child_stack_alloc += STACK_SIZE / sizeof(uint32_t);

    *--child_stack_alloc = (uint32_t)task_exit; // Return address
    *--child_stack_alloc = (uint32_t)parent->entry; // EIP

    uint32_t *child_kstack_alloc = kmalloc(STACK_SIZE);
    if (!child_kstack_alloc)
        return -1;  // error handling

    child_kstack_alloc = (uint32_t*)((uint32_t)child_kstack_alloc & 0xFFFFFFF0);
    child_kstack_alloc += STACK_SIZE / sizeof(uint32_t);

    child->kernel_stack = (uint32_t)child_kstack_alloc;
    child->pid = task_index++;

    child->cpu.esp_ = (uint32_t)child_stack_alloc;
    child->state = TASK_READY;
    memcpy(child->name, parent->name, 15);
    child->name[15] = '\0';
    child->on_exit = parent->on_exit;
    child->entry = parent->entry;
    init_signals(child);


    add_new_task(child);
    printf("Forked new task: child pid %d, parent pid %d\n", child->pid, parent->pid);
    printf("Child task: %p\n", child);

    /* 
     * In the parent, fork() returns the child's PID.
     * (The parent's saved state is unchanged so that when it resumes the fork call
     *  will see the child PID in its return value, typically placed in eax by the system call wrapper.)
     */
    return child->pid;
}



// void create_task(void (*entry)(void), const char *name, void (*on_exit)(void))
// {
//     // 1) Allocate one contiguous block: task_t + 2 stacks + alignment
//     size_t total_size = sizeof(task_t) + (2 * STACK_SIZE) + 16;
//     uint8_t *block = kmalloc(total_size);
//     if (!block)
//     {
//         printf("Allocation error in create_task\n");
//         return;
//     }
//     memset(block, 0, total_size);

//     // 2) The task structure is at the start of this block
//     task_t *task = (task_t *)block;
//     task->mem_block = block;
//     task->block_size = total_size;

//     // 3) Find an aligned address for the stacks
//     uint8_t *ptr = block + sizeof(task_t);
//     uintptr_t aligned_addr = ((uintptr_t)ptr + 15) & ~((uintptr_t)15);
//     uint8_t *stacks_base = (uint8_t *)aligned_addr;

//     // 4) Slice out the user stack
//     uint8_t  *user_stack_base = stacks_base;
//     uint32_t *user_stack_top  = (uint32_t *)(user_stack_base + STACK_SIZE);

//     // 5) Slice out the kernel stack
//     uint8_t  *kernel_stack_base = stacks_base + STACK_SIZE;
//     uint32_t *kernel_stack_top  = (uint32_t *)(kernel_stack_base + STACK_SIZE);

//     // 6) Fill task fields
//     task->pid          = task_index++;
//     task->state        = TASK_READY;
//     task->stack        = (uint32_t)user_stack_base;   // base of user stack
//     task->kernel_stack = (uint32_t)kernel_stack_top;  // top of kernel stack

//     memcpy(task->name, name, 15);
//     task->name[15] = '\0';

//     task->on_exit = on_exit;
//     init_signals(task);

//     // 7) Simulate the call stack so that the "entry" function runs first,
//     //    and when it returns, we go to "task_exit".
//     uint32_t *sim_stack = user_stack_top;
//     *--sim_stack = (uint32_t)task_exit; // Return from entry => goes here
//     *--sim_stack = (uint32_t)entry;     // Actually start at 'entry'
//     task->cpu.esp_ = (uint32_t)sim_stack;

//     // 8) Insert into the task list
//     add_new_task(task);

//     printf("Task %d created\n", task->pid);
// }

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
    init_signals(idle);
    install_all_cmds(commands, TASKS);
}

/*************************************** */

void task_write(void)
{
    puts("Task 3 Started\n");
    int i = 0;
    int return_value;
    signal(2, task_exit);
    while (1)
    {
        i++;
        // puts_color("Task 3\n", RED);
        // if (i % 1000 == 0)
        // {

        const char* msg = "Hello, world!\n";
        size_t msg_len = strlen(msg);

        // puts_color("TEST_WRITE\n", LIGHT_MAGENTA);
        // asm volatile (
        //     "mov $1, %%eax\n"
        //     "mov $1, %%ebx\n"
        //     "mov %1, %%ecx\n"
        //     "mov %2, %%edx\n"
        //     "int $0x80\n"
        //     "mov %%eax, %0\n"
        //     : "=r"(return_value)
        //     : "r"(msg), "r"(msg_len)
        //     : "eax", "ebx", "ecx", "edx"
        // );
        // printf("SYS_WRITE return value: %d\n", return_value);
        return_value = write(2, msg, msg_len);
        if (return_value <= 0)
        {
            puts_color("Error writing\n", RED);
            break;
        }
        else
        {
            printf("SYS_WRITE return value-------------: %d\n", return_value);
        }

            scheduler();
        // }
    }
}

void task_1_exit()
{
    puts_color("Task 1 exited\n", RED);
}

void task_1(void)
{
    puts("Task 1 Started\n");
    // printf("hndler: %p\n", task_1_sighandler);

    // scheduler();
    // kill(2, 2);
    // read(0, NULL, 0);
    puts("Task 1: Signal handler set\n");
    while (1)
    {
        // puts_color("Task 1\n", i %128);
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
    _signal(2, task_exit);
    printf("Task 4 Started\n");
    int i;

    i = _fork();
    // printf("Forked: %d\n", i);
    while (1)
    {
        char buffer[10];
        size_t return_value;
        // puts_color("Task 4\n", GREEN);
        return_value = read(0, buffer, sizeof(buffer));
        if (return_value <= 0)
        {
            puts_color("Error reading\n", RED);
        }
        // puts("Buffer after sys_read: '");
        // else
        // {
        //     printf("SYS_READ return value: %d\n", return_value);
        // }
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
    create_task(kshell, "kshell", NULL);
    create_task(task_1, "task_1", task_1_exit);
    create_task(task_read, "task_read", NULL);
    create_task(task_2, "task_2", task_2_exit);
    // create_task(task_write, "task_write", NULL); // floods CLI
    to_free = NULL;
    printf("current_task: %p\n", current_task);
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
