#include "signals.h"
#include "../utils/utils.h"
#include "../display/display.h"
#include "idt.h"

#define MAX_SIGNALS 32

/* TODO move it to each task as now we only have the 'core' */
signal_context_t kernel_signals;

void signal(int signal, signal_handler_t handler)
{
    if (signal >= 0 && signal < MAX_SIGNALS)
    {
        kernel_signals.handlers[signal] = handler;
    }
}

void block_signal(int signal)
{
    if (signal >= 0 && signal < MAX_SIGNALS)
    {
        kernel_signals.blocked_signals |= (1 << signal);
    }
}

void unblock_signal(int signal)
{
    if (signal >= 0 && signal < MAX_SIGNALS)
    {
        kernel_signals.blocked_signals &= ~(1 << signal);
    }
}

void handle_signals()
{
    for (int i = 0; i < MAX_SIGNALS; i++)
    {
        if ((kernel_signals.pending_signals & (1 << i)) &&\
         !(kernel_signals.blocked_signals & (1 << i)))
        {
            kernel_signals.pending_signals &= ~(1 << i);
            if (kernel_signals.handlers[i])
            {
                kernel_signals.handlers[i](i);
            }
        }
    }
}

void kill(pid_t pid, int signal)
{
    if (signal >= 0 && signal < MAX_SIGNALS)
    {
        kernel_signals.pending_signals |= (1 << signal);
    }
    handle_signals();
}

static void signal_handler(int signal)
{
    printf("Signal %d received\n", signal);
}

static void panic_signal_handler(int signal)
{
    printf("Panic signal %d received\n", signal);
    kernel_panic("Panic signal received");
}

void init_signals()
{
    for (int i = 0; i < MAX_SIGNALS; i++)
    {
        signal(i, signal_handler);
    }
    signal(0, panic_signal_handler);
    signal(6, panic_signal_handler);
    signal(14, panic_signal_handler);

    kernel_signals.pending_signals = 0;
    kernel_signals.blocked_signals = 0;
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
