#include "task.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
#include "../utils/stdint.h"
#include "../display/display.h"
#include "../keyboard/signals.h"
#include "../gdt/gdt.h"
#include "../kshell/kshell.h"
#include "../syscall_wrappers/stdlib.h"
#include "../utils/queue.h"
#include "../sockets/sockets.h"

#define STACK_SIZE 4096
#define MAX_ACTIVE_TASKS 15

void kernel_main();
void task_1(void);
void task_1_exit();
static void task_exit_pid(pid_t task_id);
static void task_exit_task(task_t* task, int singal);
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
Queue finished_pid_queue;

void dtach_from_childs(task_t* task)
{
    child_list_t *current = task->children;
    while (current)
    {
        current->task->parent = NULL;
        current = current->next;
    }
    kfree(task->children);
}

void remove_from_father(task_t* task)
{
    if (!task->parent)
        return;
    child_list_t *current = task->parent->children;
    child_list_t *prev = NULL;
    while (current)
    {
        if (current->task == task)
        {
            if (prev)
                prev->next = current->next;
            else
                task->parent->children = current->next;
            kfree(current);
            break;
        }
        prev = current;
        current = current->next;
    }
}

void free_finished_tasks()
{
    if (!to_free)
        return;
    dtach_from_childs(to_free);
    remove_from_father(to_free);
    kfree((void*)to_free->kernel_stack);
    kfree((void*)to_free->stack);
    kfree(to_free);
    to_free = NULL;
}

task_t* get_current_task()
{
    return current_task;
}

pid_t _getpid()
{
    if (!current_task)
        return 0;
    return current_task->pid;
}

uid_t get_current_uid()
{
    return current_task->uid;
}

uid_t get_current_euid()
{
    return current_task->euid;
}

gid_t get_current_gid()
{
    return current_task->gid;
}

void set_current_uid(uid_t uid)
{
    current_task->uid = uid;
}

void set_current_euid(uid_t euid)
{
    current_task->euid = euid;
}

void set_current_gid(gid_t gid)
{
    current_task->gid = gid;
}

pid_t _wait(int* status)
{
    data_t data;
    pid_t pid = dequeue(&finished_pid_queue, &data);
    if (pid == 0)
    {
        scheduler();
        while (pid == 0)
        {
            pid = dequeue(&finished_pid_queue, &data);
            if (pid == 0)
                scheduler();
        }
    }
    if (status)
        *status = data.status;
    return data.pid;
}

task_t* get_task_by_pid(pid_t pid)
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
    if (!current_task)
    {
        if (!task_list)
            return;
        current_task = task_list;
    }
    
    free_finished_tasks();

    task_t *next = current_task->next;

    if (next->pid == 0)
    {
        next = next->next;
    }

    while (next->state == TASK_WAITING)
    {
        next = next->next;
        if (next->pid == 0)
        {
            next = next->next;
        }
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
    // printf("Task %d added\n", new_task->pid);
}

static void task_exit_task(task_t* task, int signal)
{
    if (task->on_exit)
        task->on_exit();

    task_t *prev = current_task;
    while (prev->next != task)
        prev = prev->next;

    prev->next = task->next;

    pid_t pid = task->pid;

    task->state = TASK_ZOMBIE;
    // printf("Task %d exited with status %d ---\n", pid, signal);
    enqueue(&finished_pid_queue, pid, signal);
    to_free = task;

    scheduler();
}

static void task_exit_pid(pid_t task_id)
{
    task_t *task = get_task_by_pid(task_id);
    task_exit_task(task, 0);
}

/* Cb function to be called once a task returns
*/
static void task_exit()
{
    task_exit_task(current_task, 0);
}

void kill_task(int signal)
{
    // printf("Killing task %d With signal:%d--------------\n", current_task->pid, signal);
    task_exit_task(current_task, signal);
}

void add_child(task_t* parent, task_t* child)
{
    child_list_t *new_child = kmalloc(sizeof(child_list_t));
    new_child->task = child;
    new_child->next = NULL;

    if (!parent->children)
    {
        parent->children = new_child;
    }
    else
    {
        child_list_t *current = parent->children;
        while (current->next)
        {
            current = current->next;
        }
        current->next = new_child;
    }
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
    
    memset(stack, 0, STACK_SIZE);
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
    task->uid = 0;
    task->euid = 0;
    task->gid = 0;
    init_signals(task);
    add_new_task(task);
    // printf("Task %d created\n", task->pid);
}

extern void fork_trampoline(void);
extern void capture_cpu_state(cpu_state_t *state);
pid_t _do_fork(const cpu_state_t *parent_state);

pid_t _do_fork(const cpu_state_t *parent_state)
{
    if (task_index >= MAX_ACTIVE_TASKS)
    {
        puts_color("Max number of tasks reached\n", RED);
        return -1;
    }

    task_t *parent = current_task;
    task_t *child = kmalloc(sizeof(task_t));
    if (!child)
        return -1;

    uint32_t live_esp = parent_state->esp_;


    /*
     * Determine the parent's aligned stack top.
     * (This must match the way create_task() computes it.)
     */
    uint32_t *parent_raw_stack = (uint32_t *)parent->stack;
    uint32_t *parent_stack_top = (uint32_t *)((uint32_t)parent_raw_stack & 0xFFFFFFF0);
    parent_stack_top += STACK_SIZE / sizeof(uint32_t);

    /* Calculate the used portion of the parent's stack in bytes.
       (Stack grows downward so: used_bytes = parent_stack_top - live_esp) */
    size_t used_bytes = (uint32_t)parent_stack_top - live_esp;

    /*
     * Allocate and align a new user stack for the child.
     */
    uint32_t *child_raw_stack = kmalloc(STACK_SIZE);
    if (!child_raw_stack)
        return -1;
    child->stack = (uint32_t)child_raw_stack;
    uint32_t *child_stack_top = (uint32_t *)((uint32_t)child_raw_stack & 0xFFFFFFF0);
    child_stack_top += STACK_SIZE / sizeof(uint32_t);

    /*
     * Compute the child's new ESP so that the same amount of stack is used.
     * That is, if parent's ESP is X bytes below parent's top,
     * then child's ESP = child_stack_top - used_bytes.
     */
    uint32_t *child_cpu_esp = (uint32_t *)((uint32_t)child_stack_top - used_bytes);
    memcpy(child_cpu_esp, (void*)live_esp, used_bytes);

    /* 
     * Copy parent's CPU state (captured earlier) into the child.
     */
    memcpy(&child->cpu, parent_state, sizeof(cpu_state_t));
    child->cpu.esp_ = (uint32_t)child_cpu_esp;

    /* --- Adjust the frame pointer (EBP) chain in the copied stack --- */
    {
        uintptr_t parent_top_addr = (uintptr_t)parent_stack_top;
        uintptr_t child_top_addr  = (uintptr_t)child_stack_top;
        uintptr_t delta = child_top_addr - parent_top_addr;

        if (child->cpu.ebp >= live_esp && child->cpu.ebp < parent_top_addr)
        {
            child->cpu.ebp += delta;
            uint32_t *cur_ebp = (uint32_t *)child->cpu.ebp;
            while(cur_ebp &&
                  ((uintptr_t)cur_ebp >= (uintptr_t)child_cpu_esp) &&
                  ((uintptr_t)cur_ebp < child_top_addr))
            {
                uint32_t saved_ebp = *cur_ebp;
                if (saved_ebp >= live_esp && saved_ebp < parent_top_addr)
                {
                    uint32_t new_val = saved_ebp + delta;
                    *cur_ebp = new_val;
                    cur_ebp = (uint32_t *)new_val;
                }
                else
                {
                    break;
                }
            }
        }
    }
    /* --- End EBP fix-up --- */

    /*
     * Insert a trampoline address on the child's stack.
     *
     * When the child is scheduled, the context switch will load its ESP
     * from child->cpu.esp_. A subsequent "ret" will pop the trampoline address,
     * jump to fork_trampoline (which sets EAX to 0), and then "ret" to the
     * original return address.
     */
    uint32_t *child_sp = (uint32_t *)child->cpu.esp_;
    child_sp--;  // reserve space for the trampoline address
    *child_sp = (uint32_t)fork_trampoline;
    child->cpu.esp_ = (uint32_t)child_sp;

    /*
     * Allocate a new kernel stack for the child.
     * (Here we simply allocate a fresh kernel stack rather than copying the parent's.)
     */
    uint32_t *child_kstack_alloc = kmalloc(STACK_SIZE);
    if (!child_kstack_alloc)
        return -1;
    uint32_t *child_kstack_top = (uint32_t *)((uint32_t)child_kstack_alloc & 0xFFFFFFF0);
    child_kstack_top += STACK_SIZE / sizeof(uint32_t);
    child->kernel_stack = (uint32_t)child_kstack_top;

    /* Set up the remainder of the child's task structure. */
    child->pid = task_index++;
    child->state = TASK_READY;
    memcpy(child->name, parent->name, 15);
    child->name[15] = '\0';
    child->on_exit = parent->on_exit;
    child->entry = parent->entry;
    child->parent = parent;
    add_child(parent, child);
    init_signals(child);

    add_new_task(child);
    // printf("Forked new task: child pid %d, parent pid %d\n", child->pid, parent->pid);
    return child->pid;
}

pid_t _fork(void)
{
    cpu_state_t state;
    capture_cpu_state(&state);
    return _do_fork(&state);
}

void scheduler_init(void)
{
    task_t *idle = kmalloc(sizeof(task_t));
    uint32_t *stack = kmalloc(STACK_SIZE);
    stack += STACK_SIZE / sizeof(uint32_t);

    init_queue(&finished_pid_queue);

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
    idle->uid = 0;
    idle->euid = 0;
    idle->gid = 0;
    current_task = idle;
    task_list = idle;
    to_free = NULL;
    init_signals(idle);
    install_all_cmds(commands, TASKS);
}

/*************************************** */
void task_1_exit()
{
    puts_color("Task 1 exited\n", RED);
}

void task_1_sighandler(int signal)
{
    puts_color("Task 1: Signal received\n", GREEN);
}

void task_1(void)
{
    // puts("Task 1 Started\n");

    size_t mmap_size = 16 * 1024;
    void* user_buffer = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE | PROT_USER,
                             MAP_ANONYMOUS, -1, 0);
    if (user_buffer == (void*)-1)
    {
        puts_color("test_mmap: mmap failed!\n", RED);
        return;
    }

    memset(user_buffer, 'A', mmap_size);
    for (size_t i = 0; i < mmap_size; i++)
    {
        if (((char*)user_buffer)[i] != 'A')
        {
            puts_color("test_mmap: memory corruption detected!\n", RED);
            return;
        }
    }

    munmap(user_buffer, mmap_size);
    // puts_color("test_mmap: mmap/munmap test passed\n", GREEN);

    signal(2, task_1_sighandler);

    while (1)
    {
        int return_value;
        return_value = write(3, "Task 1\n", 7);
        if (return_value > 0) // it must fail so not failing it's indeed a fail
        {
            puts_color("Error writing\n", RED);
        }
        scheduler();
    }
}

void task_2_exit()
{
    // puts_color("Task 2 exited\n", RED);
}

void task_2(void)
{
    // puts("Task 2 Started\n");
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

void task_socket_send()
{
    // puts("Task SOCKSEND Started\n");
    int i = 0;
    i = 0;
    int sockfd;
    sockfd = _socket();
    /* We assume this must be 0 */
    // printf("Socket fd: %d\n", sockfd);
    while (1)
    {
        socket_send(sockfd, "Hello, world!\n", 14);

        scheduler();
    }
}

void task_socket_recv()
{
    // puts("Task SOCKRECV Started\n");

    int sockfd;
    sockfd = socket_connect(0);
    // printf("Socket fd: %d\n", sockfd);
    while (1)
    {
        char buffer[16];
        socket_recv(sockfd, buffer, sizeof(buffer));
        // puts_color("Received: ", GREEN);
        // puts_color(buffer, GREEN);
        // putc('\n');
        // puts("Task 4\n");
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
    // printf("Task 4 Started\n");
    int i;
    i = _fork();
    // printf("Forked: %d\n", i);
    if (i == 0)
    {
        // printf("My parent is %d\n", current_task->parent->pid);
        set_current_uid(1);
        set_current_euid(1);
        set_current_gid(1);
    }
    else
    {
        // printf("My child is %d\n", current_task->children->task->pid);
    }
    // printf("Task uid: %d, euid: %d. guid: %d\n", get_current_uid(), get_current_euid(), get_current_gid());
    signal(2, task_1_sighandler);
    while (1)
    {
        char buffer[10];
        int return_value;
        return_value = read(0, buffer, sizeof(buffer));
        // return_value = write(3, buffer, return_value); // error writing to fd 3
        if (return_value <= 0)
        {
            puts_color("Error reading\n", RED);
        }
        // else
        // {
        //     buffer[return_value] = '\0';
        //     puts_color("Read: ", GREEN);
        //     puts_color(buffer, GREEN);
        //     puts_color("\n", GREEN);
        //     printf("Current task: %p\n", current_task);
        // }
        scheduler();
    }
}

void task_wait()
{
    // printf("Task 5 Started\n");
    pid_t pid;
    int status;
    while (1)
    {
        pid = _wait(&status);
        set_putchar_color(GREEN);
        // printf("Task 5: Child %d exited with status %d\n", pid, status);
        set_putchar_color(LIGHT_GREY);
 
        // puts("Task 5\n");
        scheduler();
    }
}

void kshell();
void start_foo_tasks(void)
{
    create_task(kshell, "kshell", NULL);
    create_task(task_wait, "task_wait", NULL);
    create_task(task_1, "task_1", task_1_exit);
    create_task(task_read, "task_read", NULL);
    create_task(task_2, "task_2", task_2_exit);
    create_task(task_socket_send, "task_socket_send", NULL);
    create_task(task_socket_recv, "task_socket_recv", NULL);
    to_free = NULL;
    // printf("current_task: %p\n", current_task);
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
