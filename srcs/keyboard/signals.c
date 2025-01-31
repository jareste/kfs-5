#include "signals.h"
#include "../utils/utils.h"
#include "../display/display.h"
#include "../tasks/task.h"
#include "idt.h"

/* TODO move it to each task as now we only have the 'core' */

void signal_task(task_t* task, int signal, signal_handler_t handler)
{
    if (signal >= 0 && signal < MAX_SIGNALS)
    {
        task->signals.handlers[signal] = handler;
    }
}

void signal(int signal, signal_handler_t handler)
{
    signal_task(get_current_task(), signal, handler);
}

void block_signal(int signal)
{
    task_t *task = get_current_task();
    if (signal >= 0 && signal < MAX_SIGNALS)
    {
        task->signals.blocked_signals |= (1 << signal);
    }
}

void unblock_signal(int signal)
{
    task_t *task = get_current_task();
    if (signal >= 0 && signal < MAX_SIGNALS)
    {
        task->signals.blocked_signals &= ~(1 << signal);
    }
}

void handle_signals()
{
    task_t *task = get_current_task();
    if (task->signals.pending_signals == 0)
    {
        return;
    }
    for (int i = 0; i < MAX_SIGNALS; i++)
    {
        if ((task->signals.pending_signals & (1 << i)) &&\
         !(task->signals.blocked_signals & (1 << i)))
        {
            task->signals.pending_signals &= ~(1 << i);
            if (task->signals.handlers[i])
            {
                task->signals.handlers[i](i);
            }
        }
    }
}

void kill(pid_t pid, int signal)
{
    if (pid == 0)
    {
        return;
    }
    task_t *task = find_task(pid);
    if (!task)
    {
        printf("Task with PID %d not found\n", pid);
        return;
    }
    if (signal >= 0 && signal < MAX_SIGNALS)
    {
        task->signals.pending_signals |= (1 << signal);
    }
}

static void signal_handler(int signal)
{
    kill_task();
}

static void panic_signal_handler(int signal)
{
    kernel_panic("Panic signal received");
}

void init_signals(task_t* task)
{
    for (int i = 0; i < MAX_SIGNALS; i++)
    {
        task->signals.handlers[i] = NULL;
    }
    task->signals.pending_signals = 0;
    task->signals.blocked_signals = 0;

    for (int i = 0; i < MAX_SIGNALS; i++)
    {
        signal_task(task, i, signal_handler);
    }
    signal_task(task, 0, panic_signal_handler);
    signal_task(task, 6, panic_signal_handler);
    signal_task(task, 14, panic_signal_handler);

    // printf("Signal handlers set for PID %d\n", task->pid);
    // task->signals.handlers[6](6);
    task->signals.pending_signals = 0;
    task->signals.blocked_signals = 0;
}

/* This will come when i'll be having tasks. */
// void send_signal_to_task(task_t* task, int signal)
// {
//     if (signal >= 0 && signal < MAX_SIGNALS)
//     {
//         task->signals.pending_signals |= (1 << signal);
//     }
// }

// void task_signal_handler(task_t* task)
// {
//     for (int i = 0; i < MAX_SIGNALS; i++)
//     {
//         if ((task->signals.pending_signals & (1 << i)) && 
//             !(task->signals.blocked_signals & (1 << i)))
//             {
//             task->signals.pending_signals &= ~(1 << i);
//             if (task->signals.handlers[i])
//             {
//                 task->signals.handlers[i](i);
//             }
//         }
//     }
// }
